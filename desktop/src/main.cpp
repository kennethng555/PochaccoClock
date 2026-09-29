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

#include "../app/AppMode.hpp"
#include "../rendering/AppLayout.hpp"
#include "../display/SDL3Display.hpp"
#include "../clock/ClockManager.hpp"
#include "../clock/ClockTime.hpp"
#include "../clock/TimeOfDay.hpp"
#include "../music/MusicManager.hpp"
#include "../settings/Settings.hpp"
#include "../settings/SettingsRenderer.hpp"
#include "../settings/SettingsAction.hpp"
#include "../settings/SettingsActionHandler.hpp"

using namespace AppLayout;

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

    if (!TTF_Init())
    {
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
            20.0f);

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
            POCHACCO_PATH);

    if (!pochaccoTexture)
    {
        std::cerr
            << "Failed to load "
            << POCHACCO_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        TTF_CloseFont(musicBoxFont);
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
            MORNING_PATH);

    if (!morningTexture)
    {
        std::cerr
            << "Failed to load "
            << MORNING_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        SDL_DestroyTexture(pochaccoTexture);
        TTF_CloseFont(musicBoxFont);
        TTF_Quit();
        display.shutdown();

        return 1;
    }

    SDL_Texture* daytimeTexture =
        IMG_LoadTexture(
            renderer,
            DAYTIME_PATH);

    if (!daytimeTexture)
    {
        std::cerr
            << "Failed to load "
            << DAYTIME_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        SDL_DestroyTexture(morningTexture);
        SDL_DestroyTexture(pochaccoTexture);
        TTF_CloseFont(musicBoxFont);
        TTF_Quit();
        display.shutdown();

        return 1;
    }

    SDL_Texture* eveningTexture =
        IMG_LoadTexture(
            renderer,
            EVENING_PATH);

    if (!eveningTexture)
    {
        std::cerr
            << "Failed to load "
            << EVENING_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        SDL_DestroyTexture(daytimeTexture);
        SDL_DestroyTexture(morningTexture);
        SDL_DestroyTexture(pochaccoTexture);
        TTF_CloseFont(musicBoxFont);
        TTF_Quit();
        display.shutdown();

        return 1;
    }

    SDL_Texture* nightTexture =
        IMG_LoadTexture(
            renderer,
            NIGHT_PATH);

    if (!nightTexture)
    {
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
        TTF_CloseFont(musicBoxFont);
        TTF_Quit();
        display.shutdown();

        return 1;
    }

    // ========================================================
    // Clock manager
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

            settings.save(SETTINGS_PATH);
        }

        // ====================================================
        // Settings renderer
        // ====================================================

        SettingsRenderer settingsRenderer;

        if (!settingsRenderer.initialize(
                renderer,
                FONT_PATH))
        {
            SDL_Log(
                "Failed to initialize SettingsRenderer");

            return 1;
        }

        // ====================================================
        // Music manager
        // ====================================================

        MusicManager musicManager;

        if (!musicManager.initialize(renderer))
        {
            SDL_Log(
                "Failed to initialize MusicManager");

            return 1;
        }

        // ====================================================
        // Clock manager
        // ====================================================

        ClockManager clockManager(
            renderer,
            FONT_PATH);

        if (!clockManager.initialize(settings.get()))
        {
            std::cerr
                << "ClockManager initialization failed."
                << '\n';

            return 1;
        }

        // ====================================================
        // Application state
        // ====================================================

        AppMode currentMode =
            AppMode::Clock;

        bool running = true;

        // ====================================================
        // Alarm state
        // ====================================================

        int lastAlarmDay = -1;
        int lastAlarmMinute = -1;

        // ====================================================
        // Pochacco breathing animation
        // ====================================================

        float pochaccoAnimationTime = 0.0f;

        // ====================================================
        // Frame timing
        // ====================================================

        auto previousFrame =
            std::chrono::steady_clock::now();

        // ====================================================
        // Main loop
        // ====================================================

        while (running)
        {
            // ------------------------------------------------
            // Delta time
            // ------------------------------------------------

            const auto currentFrame =
                std::chrono::steady_clock::now();

            float deltaTime =
                std::chrono::duration<float>(
                    currentFrame -
                    previousFrame).count();

            previousFrame =
                currentFrame;

            if (deltaTime > 0.1f)
            {
                deltaTime = 0.1f;
            }

            // ------------------------------------------------
            // Events
            // ------------------------------------------------

            bool touchActive = false;

            float touchStartX = 0.0f;
            float touchStartY = 0.0f;

            SDL_Event event;

            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                    case SDL_EVENT_QUIT:
                    {
                        running = false;
                        break;
                    }

                    case SDL_EVENT_KEY_DOWN:
                    {
                        if (event.key.key == SDLK_M)
                        {
                            currentMode =
                                AppMode::Music;
                        }
                        else if (event.key.key == SDLK_C)
                        {
                            currentMode =
                                AppMode::Clock;
                        }
                        else if (event.key.key == SDLK_S)
                        {
                            currentMode =
                                AppMode::Settings;
                        }
                        else if (
                            currentMode == AppMode::Music &&
                            event.key.key == SDLK_SPACE)
                        {
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
                            const SDL_FRect musicButtonBounds =
                                clockManager.isBirthdayActive()
                                    ? BIRTHDAY_MUSIC_BOX_BUTTON_BOUNDS
                                    : MUSIC_BOX_BUTTON_BOUNDS;

                            const SDL_FPoint point{
                                event.button.x,
                                event.button.y
                            };

                            if (SDL_PointInRectFloat(
                                    &point,
                                    &musicButtonBounds))
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
                                    fullScreenBounds(),
                                    settings);

                            SettingsActionHandler::handle(
                                action,
                                settings,
                                settingsRenderer,
                                musicManager,
                                clockManager,
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
                            static_cast<float>(
                                LOGICAL_WIDTH);

                        touchStartY =
                            event.tfinger.y *
                            static_cast<float>(
                                LOGICAL_HEIGHT);

                        break;
                    }

                    case SDL_EVENT_FINGER_UP:
                    {
                        if (!touchActive)
                            break;

                        touchActive = false;

                        const float touchEndX =
                            event.tfinger.x *
                            static_cast<float>(
                                LOGICAL_WIDTH);

                        const float touchEndY =
                            event.tfinger.y *
                            static_cast<float>(
                                LOGICAL_HEIGHT);

                        const float deltaX =
                            touchEndX -
                            touchStartX;

                        const float deltaY =
                            touchEndY -
                            touchStartY;

                        // ----------------------------------------
                        // Horizontal swipe
                        // ----------------------------------------

                        if (
                            std::abs(deltaX) > 100.0f &&
                            std::abs(deltaX) >
                                std::abs(deltaY) * 1.5f)
                        {
                            if (deltaX < 0.0f)
                            {
                                currentMode =
                                    AppMode::Music;
                            }
                            else
                            {
                                currentMode =
                                    AppMode::Clock;
                            }

                            break;
                        }

                        // ----------------------------------------
                        // Tap
                        // ----------------------------------------

                        if (currentMode == AppMode::Music)
                        {
                            musicManager.handleTouch(
                                touchEndX,
                                touchEndY,
                                MUSIC_BOUNDS);
                        }
                        else if (currentMode == AppMode::Clock)
                        {
                            const SDL_FRect musicButtonBounds =
                                clockManager.isBirthdayActive()
                                    ? BIRTHDAY_MUSIC_BOX_BUTTON_BOUNDS
                                    : MUSIC_BOX_BUTTON_BOUNDS;

                            const SDL_FPoint point{
                                touchEndX,
                                touchEndY
                            };

                            if (SDL_PointInRectFloat(
                                    &point,
                                    &musicButtonBounds))
                            {
                                musicManager.toggleMusicBox(
                                    settings.get().music.songPath);
                            }
                        }
                        else if (currentMode == AppMode::Settings)
                        {
                            const SDL_FRect settingsBounds =
                                fullScreenBounds();

                            const SettingsAction action =
                                settingsRenderer.getAction(
                                    touchEndX,
                                    touchEndY,
                                    settingsBounds,
                                    settings);

                            SettingsActionHandler::handle(
                                action,
                                settings,
                                settingsRenderer,
                                musicManager,
                                clockManager,
                                currentMode);
                        }

                        break;
                    }
                }
            }

            // ------------------------------------------------
            // Current local time
            // ------------------------------------------------

            const auto now =
                std::chrono::system_clock::now();

            const std::time_t nowTime =
                std::chrono::system_clock::to_time_t(now);

            std::tm localTime{};

#if defined(_WIN32)

            localtime_s(
                &localTime,
                &nowTime);

#else

            localtime_r(
                &nowTime,
                &localTime);

#endif

            // ------------------------------------------------
            // Fractional seconds
            // ------------------------------------------------

            const auto milliseconds =
                std::chrono::duration_cast<
                    std::chrono::milliseconds>(
                    now.time_since_epoch())
                    .count();

            const double fractionalSecond =
                static_cast<double>(
                    milliseconds % 1000) /
                1000.0;

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
            // Birthday
            // ------------------------------------------------

            const BirthdaySettings& birthday =
                settings.get().birthday;

            const bool birthdayToday =
                birthday.enabled &&
                currentTime.month == birthday.month &&
                currentTime.day == birthday.day;

            if (
                birthdayToday &&
                !clockManager.isBirthdayActive())
            {
                SDL_Log(
                    "Birthday mode started for %s",
                    birthday.name.c_str());

                clockManager.startBirthday(
                    birthday);

                musicManager.startBirthdayMusic(
                    birthday.musicPath,
                    settings.get().music.songPath);
            }
            else if (
                !birthdayToday &&
                clockManager.isBirthdayActive())
            {
                SDL_Log(
                    "Birthday mode ended");

                clockManager.stopBirthday();

                musicManager.stopBirthdayMusic();
            }

            // ------------------------------------------------
            // Check alarms
            // ------------------------------------------------

            const int currentDay =
                localTime.tm_yday;

            const int currentMinute =
                currentTime.hour * 60 +
                currentTime.minute;

            if (
                currentDay != lastAlarmDay ||
                currentMinute != lastAlarmMinute)
            {
                for (
                    AlarmConfig& alarm :
                    settings.get().alarms)
                {
                    if (!alarm.enabled)
                        continue;

                    if (
                        alarm.hour != currentTime.hour ||
                        alarm.minute != currentTime.minute)
                    {
                        continue;
                    }

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

                    if (!alarm.animationDirectory.empty())
                    {
                        clockManager
                            .getAnimationManager()
                            .showAnimation(
                                alarm.animationDirectory);
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

            if (clockManager.isBirthdayActive())
            {
                clockManager.updateBirthday(
                    deltaTime);
            }
            else
            {
                switch (currentMode)
                {
                    case AppMode::Clock:
                    {
                        clockManager.update(
                            deltaTime,
                            settings.get(),
                            currentTime);

                        break;
                    }

                    case AppMode::Music:
                    case AppMode::Settings:
                        break;
                }
            }

            musicManager.update(deltaTime);

            pochaccoAnimationTime +=
                deltaTime;

            // ------------------------------------------------
            // Determine time of day
            // ------------------------------------------------

            const TimeOfDay timeOfDay =
                [&]()
                {
                    if (
                        currentTime.hour >= 6 &&
                        currentTime.hour < 10)
                    {
                        return TimeOfDay::Morning;
                    }

                    if (
                        currentTime.hour >= 10 &&
                        currentTime.hour < 17)
                    {
                        return TimeOfDay::Day;
                    }

                    if (
                        currentTime.hour >= 17 &&
                        currentTime.hour < 20)
                    {
                        return TimeOfDay::Evening;
                    }

                    return TimeOfDay::Night;
                }();

            // ------------------------------------------------
            // Select background
            // ------------------------------------------------

            SDL_Texture* backgroundTexture = nullptr;

            switch (timeOfDay)
            {
                case TimeOfDay::Morning:
                    backgroundTexture =
                        morningTexture;
                    break;

                case TimeOfDay::Day:
                    backgroundTexture =
                        daytimeTexture;
                    break;

                case TimeOfDay::Evening:
                    backgroundTexture =
                        eveningTexture;
                    break;

                case TimeOfDay::Night:
                    backgroundTexture =
                        nightTexture;
                    break;
            }

            // ------------------------------------------------
            // Pochacco breathing animation
            // ------------------------------------------------

            const float breathing =
                1.0f +
                0.008f *
                std::sin(
                    pochaccoAnimationTime * 2.0f);

            SDL_FRect pochaccoBounds =
                POCHACCO_BOUNDS;

            pochaccoBounds.w =
                POCHACCO_BOUNDS.w *
                breathing;

            pochaccoBounds.h =
                POCHACCO_BOUNDS.h *
                breathing;

            pochaccoBounds.x =
                POCHACCO_BOUNDS.x -
                (
                    pochaccoBounds.w -
                    POCHACCO_BOUNDS.w
                ) / 2.0f;

            pochaccoBounds.y =
                POCHACCO_BOUNDS.y -
                (
                    pochaccoBounds.h -
                    POCHACCO_BOUNDS.h
                );

            // =================================================
            // Begin frame
            // =================================================

            const SDL_Color backgroundColor{
                250,
                248,
                245,
                255
            };

            display.beginFrame(
                backgroundColor);

            // =================================================
            // Background
            // =================================================

            const SDL_FRect backgroundBounds =
                fullScreenBounds();

            if (
                currentMode == AppMode::Clock &&
                clockManager.isBirthdayActive())
            {
                clockManager.renderBirthdayBackground();
            }
            else
            {
                SDL_RenderTexture(
                    renderer,
                    backgroundTexture,
                    nullptr,
                    &backgroundBounds);

                // =================================================
                // Pochacco + bed
                // =================================================

                SDL_RenderTexture(
                    renderer,
                    pochaccoTexture,
                    nullptr,
                    &pochaccoBounds);
            }

            // =================================================
            // Current mode
            // =================================================

            switch (currentMode)
            {
                case AppMode::Clock:
                {
                    const bool birthday =
                        clockManager.isBirthdayActive();

                    const SDL_FRect clockBounds =
                        AppLayout::clockBounds(
                            birthday);

                    const SDL_FRect musicBoxButtonBounds =
                        AppLayout::musicBoxButtonBounds(
                            birthday);

                    clockManager.render(
                        currentTime,
                        clockBounds,
                        timeOfDay);

                    // Preserve existing SceneRenderer
                    // Music Box button behavior.
                    const SDL_Color buttonColor =
                        birthday
                            ? (
                                musicManager.isMusicBoxPlaying()
                                    ? BIRTHDAY_MUSIC_BOX_BUTTON_ACTIVE_COLOR
                                    : BIRTHDAY_MUSIC_BOX_BUTTON_COLOR
                            )
                            : (
                                musicManager.isMusicBoxPlaying()
                                    ? MUSIC_BOX_BUTTON_ACTIVE_COLOR
                                    : MUSIC_BOX_BUTTON_COLOR
                            );

                    const SDL_Color textColor =
                        birthday
                            ? BIRTHDAY_MUSIC_BOX_TEXT_COLOR
                            : MUSIC_BOX_TEXT_COLOR;

                    SDL_SetRenderDrawColor(
                        renderer,
                        buttonColor.r,
                        buttonColor.g,
                        buttonColor.b,
                        buttonColor.a);

                    SDL_RenderFillRect(
                        renderer,
                        &musicBoxButtonBounds);

                    const char* text =
                        musicManager.isMusicBoxPlaying()
                            ? "Music Box: ON"
                            : "Music Box";

                    SDL_Surface* surface =
                        TTF_RenderText_Blended(
                            musicBoxFont,
                            text,
                            0,
                            textColor);

                    if (surface)
                    {
                        SDL_Texture* texture =
                            SDL_CreateTextureFromSurface(
                                renderer,
                                surface);

                        if (texture)
                        {
                            SDL_FRect textBounds{
                                musicBoxButtonBounds.x +
                                    (
                                        musicBoxButtonBounds.w -
                                        static_cast<float>(
                                            surface->w)
                                    ) / 2.0f,

                                musicBoxButtonBounds.y +
                                    (
                                        musicBoxButtonBounds.h -
                                        static_cast<float>(
                                            surface->h)
                                    ) / 2.0f,

                                static_cast<float>(
                                    surface->w),

                                static_cast<float>(
                                    surface->h)
                            };

                            SDL_RenderTexture(
                                renderer,
                                texture,
                                nullptr,
                                &textBounds);

                            SDL_DestroyTexture(
                                texture);
                        }

                        SDL_DestroySurface(
                            surface);
                    }

                    break;
                }

                case AppMode::Music:
                {
                    musicManager.render(
                        renderer,
                        MUSIC_BOUNDS);

                    break;
                }

                case AppMode::Settings:
                {
                    settingsRenderer.render(
                        renderer,
                        fullScreenBounds(),
                        settings);

                    break;
                }
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