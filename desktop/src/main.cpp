#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <chrono>
#include <cmath>
#include <ctime>
#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>

#include "SDL3Display.hpp"
#include "../clock/ClockManager.hpp"
#include "../clock/ClockTime.hpp"
#include "../clock/TimeOfDay.hpp"
#include "../music/MusicManager.hpp"
#include "../settings/Settings.hpp"
#include "../settings/SettingsRenderer.hpp"
#include "../settings/SettingsAction.hpp"

enum class AppMode
{
    Clock,
    Music,
    Settings
};

// ============================================================
// Logical resolution
// ============================================================

constexpr int LOGICAL_WIDTH  = 600;
constexpr int LOGICAL_HEIGHT = 450;

// ============================================================
// Pochacco scaling
// ============================================================

constexpr float SCALE_FACTOR = 0.4f;

constexpr float POCHACCO_WIDTH =
    1370.0f * SCALE_FACTOR;

constexpr float POCHACCO_HEIGHT =
    548.0f * SCALE_FACTOR;

// ============================================================
// Permanent scene layout
// ============================================================

// Music spectrum analyzer: top-left.
constexpr SDL_FRect MUSIC_BOUNDS = {
    20.0f,    // x
    50.0f,    // y
    560.0f,   // width
    350.0f    // height
};

// Digital clock: center-left.
constexpr SDL_FRect CLOCK_BOUNDS = {
    LOGICAL_WIDTH * 0.1f,
    LOGICAL_HEIGHT * 0.25f,
    270.0f,
    230.0f
};

// ============================================================
// Music Box button
// ============================================================

constexpr SDL_FRect MUSIC_BOX_BUTTON_BOUNDS = {
    LOGICAL_WIDTH * 0.5f - 145.0f / 2.0f,
    LOGICAL_HEIGHT * 0.1f - 48.0f / 2.0f,
    145.0f,
    48.0f
};

constexpr SDL_Color MUSIC_BOX_BUTTON_COLOR = {
    184,
    220,
    180,
    255
};

constexpr SDL_Color MUSIC_BOX_BUTTON_ACTIVE_COLOR = {
    145,
    195,
    150,
    255
};

constexpr SDL_Color MUSIC_BOX_TEXT_COLOR = {
    50,
    80,
    55,
    255
};

// Pochacco + bed: bottom-right.
constexpr SDL_FRect POCHACCO_BOUNDS = {
    LOGICAL_WIDTH - POCHACCO_WIDTH + 65.0f,
    LOGICAL_HEIGHT - POCHACCO_HEIGHT,
    POCHACCO_WIDTH,
    POCHACCO_HEIGHT
};

// ============================================================
// Assets
// ============================================================

constexpr const char* POCHACCO_PATH =
    "../assets/pochacco-green.png";

constexpr const char* SIMBA_PATH =
    "../assets/Simba.gif";

constexpr const char* MORNING_PATH =
    "../assets/morning.jpg";

constexpr const char* DAYTIME_PATH =
    "../assets/daytime.jpg";

constexpr const char* EVENING_PATH =
    "../assets/evening.jpg";

constexpr const char* NIGHT_PATH =
    "../assets/night.jpg";

constexpr const char* FONT_PATH =
    "../assets/fonts/PinkyBlues.otf";

// ============================================================
// Determine time of day
// ============================================================

TimeOfDay getTimeOfDay(int hour)
{
    if (hour >= 6 && hour < 10) {
        return TimeOfDay::Morning;
    }

    if (hour >= 10 && hour < 17) {
        return TimeOfDay::Day;
    }

    if (hour >= 17 && hour < 20) {
        return TimeOfDay::Evening;
    }

    return TimeOfDay::Night;
}

// ============================================================
// Get background texture
// ============================================================

SDL_Texture* getBackgroundTexture(
    TimeOfDay timeOfDay,
    SDL_Texture* morningTexture,
    SDL_Texture* daytimeTexture,
    SDL_Texture* eveningTexture,
    SDL_Texture* nightTexture)
{
    switch (timeOfDay)
    {
        case TimeOfDay::Morning:
            return morningTexture;

        case TimeOfDay::Day:
            return daytimeTexture;

        case TimeOfDay::Evening:
            return eveningTexture;

        case TimeOfDay::Night:
            return nightTexture;
    }

    return daytimeTexture;
}

// ============================================================
// Draw Music Box button
// ============================================================

void drawMusicBoxButton(
    SDL_Renderer* renderer,
    TTF_Font* font,
    bool playing)
{
    if (!renderer || !font)
        return;

    SDL_Color buttonColor =
        playing
            ? MUSIC_BOX_BUTTON_ACTIVE_COLOR
            : MUSIC_BOX_BUTTON_COLOR;

    // Button background
    SDL_SetRenderDrawColor(
        renderer,
        buttonColor.r,
        buttonColor.g,
        buttonColor.b,
        buttonColor.a
    );

    SDL_RenderFillRect(
        renderer,
        &MUSIC_BOX_BUTTON_BOUNDS
    );

    // Button text
    const char* text =
        playing
            ? "Music Box: ON"
            : "Music Box";

    SDL_Surface* surface =
        TTF_RenderText_Blended(
            font,
            text,
            0,
            MUSIC_BOX_TEXT_COLOR
        );

    if (!surface)
        return;

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            renderer,
            surface
        );

    if (!texture)
    {
        SDL_DestroySurface(surface);
        return;
    }

    SDL_FRect textBounds{
        MUSIC_BOX_BUTTON_BOUNDS.x +
            (MUSIC_BOX_BUTTON_BOUNDS.w -
             static_cast<float>(surface->w)) / 2.0f,

        MUSIC_BOX_BUTTON_BOUNDS.y +
            (MUSIC_BOX_BUTTON_BOUNDS.h -
             static_cast<float>(surface->h)) / 2.0f,

        static_cast<float>(surface->w),
        static_cast<float>(surface->h)
    };

    SDL_RenderTexture(
        renderer,
        texture,
        nullptr,
        &textBounds
    );

    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

namespace
{
std::vector<std::string> getMusicBoxSongs()
{
    namespace fs = std::filesystem;

    std::vector<std::string> songs;

    const fs::path musicBoxDirectory =
        "../assets/music";

    if (!fs::exists(musicBoxDirectory) ||
        !fs::is_directory(musicBoxDirectory))
    {
        SDL_Log(
            "Music Box directory not found: %s",
            musicBoxDirectory.string().c_str());

        return songs;
    }

    for (const auto& entry :
         fs::directory_iterator(musicBoxDirectory))
    {
        if (!entry.is_regular_file())
            continue;

        const std::string extension =
            entry.path().extension().string();

        if (extension == ".wav" ||
            extension == ".WAV")
        {
            songs.push_back(
                entry.path().string());
        }
    }

    std::sort(
        songs.begin(),
        songs.end());

    return songs;
}

void handleSettingsAction(
    SettingsAction action,
    Settings& settings,
    SettingsRenderer& settingsRenderer,
    MusicManager& musicManager,
    AppMode& currentMode)
{
    constexpr const char* SETTINGS_PATH = "../assets/settings/settings.json";

    switch (action)
    {
        case SettingsAction::ToggleMusicLoop:
        {
            settings.get().music.loop =
                !settings.get().music.loop;

            musicManager.setLooping(
                settings.get().music.loop);

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::SelectMusicSong:
        {
            const std::vector<std::string> songs =
                getMusicBoxSongs();

            if (songs.empty())
            {
                SDL_Log(
                    "Settings: no Music Box songs found");
                break;
            }

            auto& currentSong =
                settings.get().music.songPath;

            auto currentIt =
                std::find(
                    songs.begin(),
                    songs.end(),
                    currentSong);

            if (currentIt == songs.end())
            {
                currentSong = songs.front();
            }
            else
            {
                ++currentIt;

                if (currentIt == songs.end())
                    currentIt = songs.begin();

                currentSong = *currentIt;
            }

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::SelectAlarm:
        {
            /*
             * Renderer handles the selection.
             */
            break;
        }

        case SettingsAction::AddAlarm:
        {
            AlarmConfig newAlarm;

            newAlarm.enabled = false;
            newAlarm.hour = 7;
            newAlarm.minute = 0;

            newAlarm.repeatDays = {
                true,
                true,
                true,
                true,
                true,
                false,
                false
            };

            newAlarm.playMusic = true;
            newAlarm.showAnimation = true;

            newAlarm.soundPath =
                "../assets/alarm/alarm.wav";

            settings.get().alarms.push_back(
                newAlarm);

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::ToggleAlarmEnabled:
        {
            auto& alarms = settings.get().alarms;

            const std::size_t index =
                settingsRenderer.getSelectedAlarm();

            if (index < alarms.size())
            {
                alarms[index].enabled =
                    !alarms[index].enabled;

                settings.save(SETTINGS_PATH);
            }

            break;
        }

        case SettingsAction::AdjustAlarmHour:
        {
            auto& alarms =
                settings.get().alarms;

            const std::size_t index =
                settingsRenderer.getSelectedAlarm();

            if (index < alarms.size())
            {
                AlarmConfig& alarm =
                    alarms[index];

                const bool isPM =
                    alarm.hour >= 12;

                int displayHour =
                    alarm.hour % 12;

                if (displayHour == 0)
                    displayHour = 12;

                displayHour++;

                if (displayHour > 12)
                    displayHour = 1;

                if (isPM)
                {
                    alarm.hour =
                        displayHour == 12
                            ? 12
                            : displayHour + 12;
                }
                else
                {
                    alarm.hour =
                        displayHour == 12
                            ? 0
                            : displayHour;
                }
            }

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::ToggleAlarmAmPm:
        {
            auto& alarms =
                settings.get().alarms;

            const std::size_t index =
                settingsRenderer.getSelectedAlarm();

            if (index < alarms.size())
            {
                AlarmConfig& alarm =
                    alarms[index];

                if (alarm.hour >= 12)
                {
                    // PM -> AM
                    alarm.hour -= 12;
                }
                else
                {
                    // AM -> PM
                    alarm.hour += 12;
                }
            }

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::AdjustAlarmMinute:
        {
            auto& alarms =
                settings.get().alarms;

            const std::size_t index =
                settingsRenderer.getSelectedAlarm();

            if (index < alarms.size())
            {
                alarms[index].minute =
                    (alarms[index].minute + 5) % 60;
            }

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::ToggleAlarmDay:
        {
            auto& alarms =
                settings.get().alarms;

            const std::size_t alarmIndex =
                settingsRenderer.getSelectedAlarm();

            const int day =
                settingsRenderer.getSelectedDay();

            if (alarmIndex < alarms.size() &&
                day >= 0 &&
                day < 7)
            {
                alarms[alarmIndex]
                    .repeatDays[day] =
                    !alarms[alarmIndex]
                        .repeatDays[day];
            }

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::ToggleAlarmAnimation:
        {
            auto& alarms =
                settings.get().alarms;

            const std::size_t index =
                settingsRenderer.getSelectedAlarm();

            if (index < alarms.size())
            {
                alarms[index].showAnimation =
                    !alarms[index].showAnimation;
            }

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::SelectAlarmSound:
        {
            /*
            * Renderer has already entered the
            * alarm sound selection screen.
            */
            break;
        }

        case SettingsAction::SelectAlarmSoundItem:
        {
            auto& alarms =
                settings.get().alarms;

            const std::size_t index =
                settingsRenderer.getSelectedAlarm();

            if (index < alarms.size())
            {
                alarms[index].soundPath =
                    settingsRenderer.getSelectedAlarmSound();
            }

            settingsRenderer.finishAlarmSoundSelection();

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::DeleteAlarm:
        {
            auto& alarms =
                settings.get().alarms;

            const std::size_t index =
                settingsRenderer.getSelectedAlarm();

            if (index < alarms.size())
            {
                alarms.erase(
                    alarms.begin() + index);

                settings.save(SETTINGS_PATH);
            }

            break;
        }

        case SettingsAction::AlarmBack:
            break;

        case SettingsAction::Back:
            currentMode = AppMode::Clock;
            break;

        case SettingsAction::None:
        default:
            break;
    }
}
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // ========================================================
    // Display
    // ========================================================

    SDL3Display display;

    if (!display.initialize(
            "Pochacco Clock",
            LOGICAL_WIDTH,
            LOGICAL_HEIGHT))
    {
        return 1;
    }

    SDL_Renderer* renderer =
        display.renderer();

    // ========================================================
    // SDL_ttf
    // ========================================================

    if (!TTF_Init()) {

        std::cerr
            << "TTF_Init failed: "
            << SDL_GetError()
            << '\n';

        display.shutdown();

        return 1;
    }

    TTF_Font* musicBoxFont =
    TTF_OpenFont(
        FONT_PATH,
        20.0f
    );

    if (!musicBoxFont)
    {
        std::cerr
            << "Failed to load Music Box font: "
            << SDL_GetError()
            << '\n';

        TTF_Quit();
        display.shutdown();

        return 1;
    }

    // ========================================================
    // Pochacco + bed
    // ========================================================

    SDL_Texture* pochaccoTexture =
        IMG_LoadTexture(
            renderer,
            POCHACCO_PATH
        );

    if (!pochaccoTexture) {

        std::cerr
            << "Failed to load "
            << POCHACCO_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        TTF_Quit();
        display.shutdown();

        return 1;
    }

    // ========================================================
    // Background textures
    // ========================================================

    SDL_Texture* morningTexture =
        IMG_LoadTexture(
            renderer,
            MORNING_PATH
        );

    if (!morningTexture) {

        std::cerr
            << "Failed to load "
            << MORNING_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        SDL_DestroyTexture(pochaccoTexture);

        TTF_Quit();
        display.shutdown();

        return 1;
    }

    SDL_Texture* daytimeTexture =
        IMG_LoadTexture(
            renderer,
            DAYTIME_PATH
        );

    if (!daytimeTexture) {

        std::cerr
            << "Failed to load "
            << DAYTIME_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        SDL_DestroyTexture(morningTexture);
        SDL_DestroyTexture(pochaccoTexture);

        TTF_Quit();
        display.shutdown();

        return 1;
    }

    SDL_Texture* eveningTexture =
        IMG_LoadTexture(
            renderer,
            EVENING_PATH
        );

    if (!eveningTexture) {

        std::cerr
            << "Failed to load "
            << EVENING_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        SDL_DestroyTexture(daytimeTexture);
        SDL_DestroyTexture(morningTexture);
        SDL_DestroyTexture(pochaccoTexture);

        TTF_Quit();
        display.shutdown();

        return 1;
    }

    SDL_Texture* nightTexture =
        IMG_LoadTexture(
            renderer,
            NIGHT_PATH
        );

    if (!nightTexture) {

        std::cerr
            << "Failed to load "
            << NIGHT_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        SDL_DestroyTexture(eveningTexture);
        SDL_DestroyTexture(daytimeTexture);
        SDL_DestroyTexture(morningTexture);
        SDL_DestroyTexture(pochaccoTexture);

        TTF_Quit();
        display.shutdown();

        return 1;
    }

    // ========================================================
    // Clock manager
    // ========================================================
    //
    // The scope is intentional.
    //
    // ClockManager and its renderer objects are destroyed
    // BEFORE SDL_ttf is shut down and BEFORE the SDL renderer
    // is destroyed.
    //
    // ========================================================

    {
        Settings settings;

        const std::string SETTINGS_PATH =
            "../assets/settings/settings.json";

        if (settings.load(SETTINGS_PATH))
        {
            SDL_Log(
                "Settings loaded from %s",
                SETTINGS_PATH.c_str());
        }
        else
        {
            SDL_Log(
                "No saved settings found. Using defaults.");

            /*
            * Create the initial settings file.
            */
            settings.save(SETTINGS_PATH);
        }

        // ========================================================
        // Settings renderer
        // ========================================================

        SettingsRenderer settingsRenderer;

        if (!settingsRenderer.initialize(
                renderer,
                FONT_PATH))
        {
            SDL_Log(
                "Failed to initialize SettingsRenderer");

            return 1;
        }

        // ========================================================
        // Music manager
        // ========================================================
        MusicManager musicManager;

        if (!musicManager.initialize(renderer))
        {
            SDL_Log("Failed to initialize MusicManager");
            return 1;
        }

        // ========================================================
        // Clock manager
        // ========================================================
        
        ClockManager clockManager(
            renderer,
            FONT_PATH
        );

        if (!clockManager.initialize(settings.get())) {

            std::cerr << "ClockManager initialization failed." << '\n';

            SDL_DestroyTexture(nightTexture);
            SDL_DestroyTexture(eveningTexture);
            SDL_DestroyTexture(daytimeTexture);
            SDL_DestroyTexture(morningTexture);
            SDL_DestroyTexture(pochaccoTexture);

            TTF_Quit();
            display.shutdown();

            return 1;
        }

        // ====================================================
        // Application state
        // ====================================================

        AppMode currentMode = AppMode::Clock;

        bool running = true;

        // ====================================================
        // Alarm state
        // ====================================================

        int lastAlarmDay = -1;
        int lastAlarmMinute = -1;

        // Pochacco breathing animation.
        float pochaccoAnimationTime = 0.0f;

        // ====================================================
        // Frame timing
        // ====================================================

        auto previousFrame =
            std::chrono::steady_clock::now();

        // ====================================================
        // Main loop
        // ====================================================

        while (running) {

            // ------------------------------------------------
            // Delta time
            // ------------------------------------------------

            const auto currentFrame =
                std::chrono::steady_clock::now();

            float deltaTime =
                std::chrono::duration<float>(
                    currentFrame -
                    previousFrame
                ).count();

            previousFrame =
                currentFrame;

            // Prevent huge animation jumps.
            if (deltaTime > 0.1f) {
                deltaTime = 0.1f;
            }

            // ------------------------------------------------
            // Events
            // ------------------------------------------------

            bool touchActive = false;
            float touchStartX = 0.0f;
            float touchStartY = 0.0f;
            SDL_Event event;

            while (SDL_PollEvent(&event)) {

                switch (event.type) {

                    case SDL_EVENT_QUIT:

                        running = false;
                        break;

                    case SDL_EVENT_KEY_DOWN:
                    {
                        if (event.key.key == SDLK_M)
                        {
                            currentMode = AppMode::Music;
                        }
                        else if (event.key.key == SDLK_C)
                        {
                            currentMode = AppMode::Clock;
                        }
                        else if (event.key.key == SDLK_S)
                        {
                            currentMode = AppMode::Settings;
                        }
                        else if (currentMode == AppMode::Music &&
                                event.key.key == SDLK_SPACE)
                        {
                            // Temporary playback toggle.
                            musicManager.togglePlayPause();
                        }

                        break;
                    }

                    case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    {
                        if (currentMode == AppMode::Music)
                        {
                            musicManager.handleMouseClick(
                                event.button.x,
                                event.button.y,
                                MUSIC_BOUNDS);
                        }
                        else if (currentMode == AppMode::Clock)
                        {
                            const SDL_FPoint point{
                                event.button.x,
                                event.button.y
                            };

                            if (SDL_PointInRectFloat(
                                    &point,
                                    &MUSIC_BOX_BUTTON_BOUNDS))
                            {
                                musicManager.toggleMusicBox(
                                    settings.get().music.songPath);
                            }
                        }
                        else if (currentMode == AppMode::Settings)
                        {
                            const SettingsAction action =
                                settingsRenderer.getAction(
                                    event.button.x,
                                    event.button.y,
                                    SDL_FRect{
                                        0.0f,
                                        0.0f,
                                        static_cast<float>(LOGICAL_WIDTH),
                                        static_cast<float>(LOGICAL_HEIGHT)
                                    },
                                    settings);

                            handleSettingsAction(
                                action,
                                settings,
                                settingsRenderer,
                                musicManager,
                                currentMode);
                        }

                        break;
                    }

                    case SDL_EVENT_MOUSE_WHEEL:
                    {
                        if (currentMode == AppMode::Settings)
                        {
                            settingsRenderer.scrollAlarms(
                                -event.wheel.y * 35.0f);
                        }

                        break;
                    }

                    case SDL_EVENT_FINGER_DOWN:
                    {
                        touchActive = true;

                        touchStartX =
                            event.tfinger.x *
                            static_cast<float>(LOGICAL_WIDTH);

                        touchStartY =
                            event.tfinger.y *
                            static_cast<float>(LOGICAL_HEIGHT);

                        break;
                    }

                    case SDL_EVENT_FINGER_UP:
                    {
                        if (!touchActive)
                            break;

                        touchActive = false;

                        const float touchEndX =
                            event.tfinger.x *
                            static_cast<float>(LOGICAL_WIDTH);

                        const float touchEndY =
                            event.tfinger.y *
                            static_cast<float>(LOGICAL_HEIGHT);

                        const float deltaX =
                            touchEndX - touchStartX;

                        const float deltaY =
                            touchEndY - touchStartY;

                        /*
                        * Only treat a mostly-horizontal movement as
                        * a mode-changing swipe.
                        */
                        if (std::abs(deltaX) > 100.0f &&
                            std::abs(deltaX) > std::abs(deltaY) * 1.5f)
                        {
                            if (deltaX < 0.0f)
                            {
                                // Swipe left
                                currentMode = AppMode::Music;
                            }
                            else
                            {
                                // Swipe right
                                currentMode = AppMode::Clock;
                            }

                            break;
                        }

                        /*
                        * Otherwise this was a tap.
                        */
                        if (currentMode == AppMode::Music)
                        {
                            musicManager.handleTouch(
                                touchEndX,
                                touchEndY,
                                MUSIC_BOUNDS);
                        }
                        else if (currentMode == AppMode::Clock)
                        {
                            const SDL_FPoint point{
                                touchEndX,
                                touchEndY
                            };

                            if (SDL_PointInRectFloat(
                                    &point,
                                    &MUSIC_BOX_BUTTON_BOUNDS))
                            {
                                musicManager.toggleMusicBox(
                                    settings.get().music.songPath);
                            }
                        }
                        else if (currentMode == AppMode::Settings)
                        {
                            const SDL_FRect settingsBounds{
                                0.0f,
                                0.0f,
                                static_cast<float>(LOGICAL_WIDTH),
                                static_cast<float>(LOGICAL_HEIGHT)
                            };

                            const SettingsAction action =
                                settingsRenderer.getAction(
                                    touchEndX,
                                    touchEndY,
                                    settingsBounds,
                                    settings);

                            handleSettingsAction(
                                action,
                                settings,
                                settingsRenderer,
                                musicManager,
                                currentMode);
                        }

                        break;
                    }
                }
            }

            // ------------------------------------------------
            // Get current local time
            // ------------------------------------------------

            const auto now =
                std::chrono::system_clock::now();

            const std::time_t nowTime =
                std::chrono::system_clock::to_time_t(
                    now
                );

            std::tm localTime{};

#if defined(_WIN32)

            localtime_s(
                &localTime,
                &nowTime
            );

#else

            localtime_r(
                &nowTime,
                &localTime
            );

#endif

            // ------------------------------------------------
            // Fractional seconds
            // ------------------------------------------------

            const auto milliseconds =
                std::chrono::duration_cast<
                    std::chrono::milliseconds
                >(
                    now.time_since_epoch()
                ).count();

            const double fractionalSecond =
                static_cast<double>(
                    milliseconds % 1000
                ) / 1000.0;

            // ------------------------------------------------
            // ClockTime
            // ------------------------------------------------

            ClockTime currentTime{
                localTime.tm_year + 1900,
                localTime.tm_mon + 1,
                localTime.tm_mday,

                localTime.tm_hour,
                localTime.tm_min,
                localTime.tm_sec,

                fractionalSecond
            };

            // ------------------------------------------------
            // Check alarms
            // ------------------------------------------------

            const int currentDay =
                localTime.tm_yday;

            const int currentMinute =
                currentTime.hour * 60 +
                currentTime.minute;

            /*
            * Only evaluate alarms once per minute.
            *
            * tm_yday is used instead of tm_wday because it
            * uniquely identifies the day within the year.
            */
            if (currentDay != lastAlarmDay ||
                currentMinute != lastAlarmMinute)
            {
                for (AlarmConfig& alarm :
                    settings.get().alarms)
                {
                    if (!alarm.enabled)
                        continue;

                    if (alarm.hour != currentTime.hour ||
                        alarm.minute != currentTime.minute)
                    {
                        continue;
                    }

                    /*
                    * tm_wday:
                    *
                    * Sunday    = 0
                    * Monday    = 1
                    * ...
                    * Saturday  = 6
                    */
                    const int weekday =
                        localTime.tm_wday;

                    if (!alarm.repeatDays[weekday])
                        continue;

                    SDL_Log(
                        "Alarm triggered: %02d:%02d",
                        alarm.hour,
                        alarm.minute);

                    // --------------------------------------------
                    // Alarm animation
                    // --------------------------------------------

                    if (alarm.showAnimation)
                    {
                        clockManager
                            .getAnimationManager()
                            .showAnimation(0);
                    }

                    // --------------------------------------------
                    // Alarm sound
                    // --------------------------------------------

                    if (alarm.playMusic)
                    {
                        musicManager.playSound(
                            alarm.soundPath,
                            settings.get().music.songPath);
                    }
                }

                lastAlarmDay = currentDay;
                lastAlarmMinute = currentMinute;
            }

            // ------------------------------------------------
            // Update
            // ------------------------------------------------

            switch (currentMode)
            {
                case AppMode::Clock:
                    clockManager.update(
                        deltaTime,
                        settings.get(),
                        currentTime);
                    break;

                case AppMode::Music:
                    break;

                case AppMode::Settings:
                    break;
            }

            musicManager.update(deltaTime);

            pochaccoAnimationTime +=
                deltaTime;

            // ------------------------------------------------
            // Determine time of day
            // ------------------------------------------------

            const TimeOfDay timeOfDay =
                getTimeOfDay(
                    currentTime.hour
                );

            // ------------------------------------------------
            // Select background
            // ------------------------------------------------

            SDL_Texture* backgroundTexture =
                getBackgroundTexture(
                    timeOfDay,
                    morningTexture,
                    daytimeTexture,
                    eveningTexture,
                    nightTexture
                );

            // ------------------------------------------------
            // Pochacco breathing animation
            // ------------------------------------------------

            const float breathing =
                1.0f +
                0.008f *
                std::sin(
                    pochaccoAnimationTime *
                    2.0f
                );

            SDL_FRect pochaccoBounds =
                POCHACCO_BOUNDS;

            pochaccoBounds.w =
                POCHACCO_BOUNDS.w *
                breathing;

            pochaccoBounds.h =
                POCHACCO_BOUNDS.h *
                breathing;

            // Keep Pochacco centered horizontally.
            pochaccoBounds.x =
                POCHACCO_BOUNDS.x -
                (
                    pochaccoBounds.w -
                    POCHACCO_BOUNDS.w
                ) / 2.0f;

            // Keep the bottom of the bed anchored.
            pochaccoBounds.y =
                POCHACCO_BOUNDS.y -
                (
                    pochaccoBounds.h -
                    POCHACCO_BOUNDS.h
                );

            // =================================================
            // Begin frame
            // =================================================

            SDL_Color backgroundColor{
                250,
                248,
                245,
                255
            };

            display.beginFrame(
                backgroundColor
            );

            // =================================================
            // Background image
            // =================================================

            SDL_FRect backgroundBounds{
                0.0f,
                0.0f,
                static_cast<float>(
                    LOGICAL_WIDTH
                ),
                static_cast<float>(
                    LOGICAL_HEIGHT
                )
            };

            SDL_RenderTexture(
                renderer,
                backgroundTexture,
                nullptr,
                &backgroundBounds
            );

            // =================================================
            // Pochacco + bed
            // =================================================

            SDL_RenderTexture(
                renderer,
                pochaccoTexture,
                nullptr,
                &pochaccoBounds
            );

            
            // =================================================
            // Render current mode
            // =================================================
            switch (currentMode)
            {
                // =================================================
                // Digital clock
                // =================================================
                case AppMode::Clock:
                    clockManager.render(
                        currentTime,
                        CLOCK_BOUNDS,
                        timeOfDay);
                    drawMusicBoxButton(
                        renderer,
                        musicBoxFont,
                        musicManager.isMusicBoxPlaying()
                    );

                    break;
                
                // =================================================
                // Spectrum analyzer
                // =================================================
                case AppMode::Music:
                    musicManager.render(
                        renderer,
                        MUSIC_BOUNDS);
                    break;

                // =================================================
                // Settings
                // =================================================
                case AppMode::Settings:
                    settingsRenderer.render(
                        renderer,
                        SDL_FRect{
                            0.0f,
                            0.0f,
                            static_cast<float>(LOGICAL_WIDTH),
                            static_cast<float>(LOGICAL_HEIGHT)
                        },
                        settings);

                    break;
            }

            // =================================================
            // Present
            // =================================================

            display.endFrame();
        }
    }

    // ========================================================
    // Cleanup
    // ========================================================

    SDL_DestroyTexture(nightTexture);
    SDL_DestroyTexture(eveningTexture);
    SDL_DestroyTexture(daytimeTexture);
    SDL_DestroyTexture(morningTexture);
    SDL_DestroyTexture(pochaccoTexture);
    TTF_CloseFont(musicBoxFont);

    TTF_Quit();

    display.shutdown();

    return 0;
}