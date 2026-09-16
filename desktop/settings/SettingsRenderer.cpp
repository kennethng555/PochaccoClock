#include "SettingsRenderer.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
constexpr int TITLE_FONT_SIZE = 32;
constexpr int OPTION_FONT_SIZE = 21;
constexpr int SMALL_FONT_SIZE = 16;

constexpr float ROW_HEIGHT = 55.0f;
constexpr float ALARM_ROW_HEIGHT = 65.0f;

constexpr SDL_Color TEXT_COLOR{
    0,
    0,
    0,
    255
};

constexpr SDL_Color MUTED_COLOR{
    90,
    90,
    90,
    255
};

std::string formatAlarmTime(
    int hour,
    int minute)
{
    const bool isPM = hour >= 12;

    int displayHour = hour % 12;

    if (displayHour == 0)
    {
        displayHour = 12;
    }

    char buffer[32];

    std::snprintf(
        buffer,
        sizeof(buffer),
        "%d:%02d %s",
        displayHour,
        minute,
        isPM ? "PM" : "AM");

    return buffer;
}

std::string getFileName(
    const std::string& path)
{
    const std::size_t slash =
        path.find_last_of("/\\");

    if (slash == std::string::npos)
    {
        return path;
    }

    return path.substr(slash + 1);
}

std::vector<std::string> getAlarmSounds()
{
    std::vector<std::string> sounds;

    const std::filesystem::path directory =
        "../assets/alarm";

    if (!std::filesystem::exists(directory) ||
        !std::filesystem::is_directory(directory))
    {
        return sounds;
    }

    for (const auto& entry :
         std::filesystem::directory_iterator(directory))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const std::string extension =
            entry.path().extension().string();

        if (extension == ".wav" ||
            extension == ".WAV")
        {
            sounds.push_back(
                entry.path().string());
        }
    }

    std::sort(
        sounds.begin(),
        sounds.end());

    return sounds;
}

/*
 * ============================================================
 * Alarm animations
 * ============================================================
 *
 * Each immediate subdirectory of:
 *
 *     ../assets/animations
 *
 * is treated as an animation.
 *
 * Example:
 *
 *     ../assets/animations/Simba
 *     ../assets/animations/Toto
 *
 */
std::vector<std::string> getAlarmAnimations()
{
    std::vector<std::string> animations;

    const std::filesystem::path directory =
        "../assets/animations";

    if (!std::filesystem::exists(directory) ||
        !std::filesystem::is_directory(directory))
    {
        return animations;
    }

    for (const auto& entry :
         std::filesystem::directory_iterator(directory))
    {
        if (!entry.is_directory())
        {
            continue;
        }

        animations.push_back(
            entry.path().string());
    }

    std::sort(
        animations.begin(),
        animations.end());

    return animations;
}
}

// ============================================================
// Destructor
// ============================================================

SettingsRenderer::~SettingsRenderer()
{
    if (titleFont_)
    {
        TTF_CloseFont(titleFont_);
        titleFont_ = nullptr;
    }

    if (optionFont_)
    {
        TTF_CloseFont(optionFont_);
        optionFont_ = nullptr;
    }

    if (smallFont_)
    {
        TTF_CloseFont(smallFont_);
        smallFont_ = nullptr;
    }
}

// ============================================================
// Initialize
// ============================================================

bool SettingsRenderer::initialize(
    SDL_Renderer* renderer,
    const std::string& fontPath)
{
    if (renderer == nullptr)
    {
        return false;
    }

    titleFont_ =
        TTF_OpenFont(
            fontPath.c_str(),
            TITLE_FONT_SIZE);

    optionFont_ =
        TTF_OpenFont(
            fontPath.c_str(),
            OPTION_FONT_SIZE);

    smallFont_ =
        TTF_OpenFont(
            fontPath.c_str(),
            SMALL_FONT_SIZE);

    if (!titleFont_ ||
        !optionFont_ ||
        !smallFont_)
    {
        SDL_Log(
            "SettingsRenderer: failed to load fonts");

        return false;
    }

    initialized_ = true;

    return true;
}

// ============================================================
// Render
// ============================================================

void SettingsRenderer::render(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds,
    const Settings& settings)
{
    if (!initialized_)
    {
        return;
    }

    SDL_SetRenderClipRect(
        renderer,
        nullptr);

    const ClockSettings& config =
        settings.get();

    /*
     * Background.
     */
    SDL_SetRenderDrawColor(
        renderer,
        255,
        255,
        255,
        235);

    SDL_RenderFillRect(
        renderer,
        &bounds);

    /*
     * If the selected alarm was deleted,
     * return to the main settings page.
     */
    if ((editingAlarm_ ||
         selectingAlarmSound_ ||
         selectingAlarmAnimation_) &&
        selectedAlarm_ >= config.alarms.size())
    {
        editingAlarm_ = false;
        selectingAlarmSound_ = false;
        selectingAlarmAnimation_ = false;

        selectedAlarmSound_.clear();
        selectedAlarmAnimation_.clear();

        selectedAlarm_ = 0;
    }

    /*
     * ========================================================
     * Alarm sound selector
     * ========================================================
     */
    if (selectingAlarmSound_)
    {
        if (selectedAlarm_ < config.alarms.size())
        {
            renderAlarmSoundSelector(
                renderer,
                bounds,
                config.alarms[selectedAlarm_]);
        }

        SDL_SetRenderClipRect(
            renderer,
            nullptr);

        return;
    }

    /*
     * ========================================================
     * Alarm animation selector
     * ========================================================
     */
    if (selectingAlarmAnimation_)
    {
        if (selectedAlarm_ < config.alarms.size())
        {
            renderAlarmAnimationSelector(
                renderer,
                bounds,
                config.alarms[selectedAlarm_]);
        }

        SDL_SetRenderClipRect(
            renderer,
            nullptr);

        return;
    }

    /*
     * ========================================================
     * Alarm editor
     * ========================================================
     */
    if (editingAlarm_)
    {
        renderAlarmEditor(
            renderer,
            bounds,
            config.alarms[selectedAlarm_]);

        SDL_SetRenderClipRect(
            renderer,
            nullptr);

        return;
    }

    /*
     * ========================================================
     * Main settings
     * ========================================================
     */
    renderMainSettings(
        renderer,
        bounds,
        config);

    SDL_SetRenderClipRect(
        renderer,
        nullptr);
}

// ============================================================
// Main settings
// ============================================================

void SettingsRenderer::renderMainSettings(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds,
    const ClockSettings& config)
{
    SDL_SetRenderClipRect(
        renderer,
        nullptr);

    /*
     * ========================================================
     * Fixed Settings title
     * ========================================================
     */
    drawText(
        renderer,
        titleFont_,
        "Settings",
        bounds.x + 25.0f,
        bounds.y + 20.0f,
        TEXT_COLOR);

    /*
     * ========================================================
     * Scroll viewport
     * ========================================================
     */
    const float scrollTop =
        bounds.y + 60.0f;

    const float scrollBottom =
        bounds.y + bounds.h - 65.0f;

    SDL_FRect scrollBounds{
        bounds.x,
        scrollTop,
        bounds.w,
        scrollBottom - scrollTop
    };

    /*
     * ========================================================
     * Content positions
     * ========================================================
     */

    const float musicY =
        bounds.y + 70.0f;

    const float firstRowY =
        bounds.y + 110.0f;

    const float songY =
        firstRowY + ROW_HEIGHT;

    const float alarmsTitleY =
        songY + 65.0f;

    const float alarmStartY =
        alarmsTitleY + 50.0f;

    const float addAlarmY =
        alarmStartY +
        static_cast<float>(config.alarms.size()) *
            ALARM_ROW_HEIGHT +
        10.0f;

    /*
     * Debug section immediately follows Add Alarm.
     */
    const float debugTitleY =
        addAlarmY + 50.0f;

    const float debugAnimationY =
        debugTitleY + 35.0f;

    /*
     * Include the Debug section in the scrollable
     * content height.
     */
    const float contentBottom =
        debugAnimationY + 50.0f;

    const float contentTop =
        scrollTop;

    const float contentHeight =
        contentBottom - contentTop;

    const float maxScroll =
        std::max(
            0.0f,
            contentHeight - scrollBounds.h);

    alarmScrollOffset_ =
        std::clamp(
            alarmScrollOffset_,
            0.0f,
            maxScroll);

    const float scrollOffset =
        alarmScrollOffset_;

    /*
     * ========================================================
     * Clip scrollable content
     * ========================================================
     */
    SDL_Rect scrollClip{
        static_cast<int>(scrollBounds.x),
        static_cast<int>(scrollBounds.y),
        static_cast<int>(scrollBounds.w),
        static_cast<int>(scrollBounds.h)
    };

    SDL_SetRenderClipRect(
        renderer,
        &scrollClip);

    /*
     * ========================================================
     * Music
     * ========================================================
     */
    drawText(
        renderer,
        optionFont_,
        "Music",
        bounds.x + 30.0f,
        musicY - scrollOffset,
        TEXT_COLOR);

    /*
     * Music Box Loop.
     */
    const float visibleFirstRowY =
        firstRowY - scrollOffset;

    drawText(
        renderer,
        optionFont_,
        "MusicBox Loop",
        bounds.x + 40.0f,
        visibleFirstRowY,
        TEXT_COLOR);

    SDL_FRect loopToggle{
        bounds.x + bounds.w - 105.0f,
        visibleFirstRowY - 4.0f,
        70.0f,
        32.0f
    };

    drawToggle(
        renderer,
        loopToggle,
        config.music.loop);

    /*
     * ========================================================
     * Music Box Song
     * ========================================================
     */
    const float visibleSongY =
        songY - scrollOffset;

    drawText(
        renderer,
        optionFont_,
        "Music Box Song",
        bounds.x + 40.0f,
        visibleSongY,
        TEXT_COLOR);

    drawText(
        renderer,
        smallFont_,
        getFileName(config.music.songPath),
        bounds.x + 40.0f,
        visibleSongY + 27.0f,
        MUTED_COLOR);

    drawText(
        renderer,
        optionFont_,
        ">",
        bounds.x + bounds.w - 45.0f,
        visibleSongY + 5.0f,
        TEXT_COLOR);

    /*
     * ========================================================
     * Alarms title
     * ========================================================
     */
    const float visibleAlarmsTitleY =
        alarmsTitleY - scrollOffset;

    drawText(
        renderer,
        optionFont_,
        "Alarms",
        bounds.x + 30.0f,
        visibleAlarmsTitleY,
        TEXT_COLOR);

    /*
     * ========================================================
     * Alarm rows
     * ========================================================
     */
    const float visibleAlarmStartY =
        alarmStartY - scrollOffset;

    for (std::size_t i = 0;
         i < config.alarms.size();
         ++i)
    {
        const float alarmY =
            visibleAlarmStartY +
            static_cast<float>(i) *
                ALARM_ROW_HEIGHT;

        const AlarmConfig& alarm =
            config.alarms[i];

        drawText(
            renderer,
            optionFont_,
            "Alarm " +
                std::to_string(i + 1),
            bounds.x + 40.0f,
            alarmY,
            TEXT_COLOR);

        drawText(
            renderer,
            smallFont_,
            formatAlarmTime(
                alarm.hour,
                alarm.minute),
            bounds.x + 40.0f,
            alarmY + 26.0f,
            MUTED_COLOR);

        SDL_FRect toggleBounds{
            bounds.x + bounds.w - 105.0f,
            alarmY - 4.0f,
            70.0f,
            32.0f
        };

        drawToggle(
            renderer,
            toggleBounds,
            alarm.enabled);

        drawText(
            renderer,
            optionFont_,
            ">",
            bounds.x + bounds.w - 45.0f,
            alarmY + 5.0f,
            TEXT_COLOR);
    }

    /*
     * ========================================================
     * Add Alarm
     * ========================================================
     */
    const float visibleAddAlarmY =
        addAlarmY - scrollOffset;

    SDL_FRect addAlarmBounds{
        bounds.x + 30.0f,
        visibleAddAlarmY,
        160.0f,
        40.0f
    };

    drawText(
        renderer,
        optionFont_,
        "+ Add Alarm",
        addAlarmBounds.x + 5.0f,
        addAlarmBounds.y + 5.0f,
        TEXT_COLOR);

    /*
     * ========================================================
     * Debug
     * ========================================================
     */
    const float visibleDebugTitleY =
        debugTitleY - scrollOffset;

    const float visibleDebugAnimationY =
        debugAnimationY - scrollOffset;

    drawText(
        renderer,
        optionFont_,
        "Debug",
        bounds.x + 30.0f,
        visibleDebugTitleY,
        TEXT_COLOR);

    drawText(
        renderer,
        optionFont_,
        "Show Animation",
        bounds.x + 40.0f,
        visibleDebugAnimationY,
        TEXT_COLOR);

    drawText(
        renderer,
        optionFont_,
        ">",
        bounds.x + bounds.w - 45.0f,
        visibleDebugAnimationY,
        TEXT_COLOR);

    /*
     * ========================================================
     * Stop clipping before Back
     * ========================================================
     */
    SDL_SetRenderClipRect(
        renderer,
        nullptr);

    /*
     * ========================================================
     * Fixed Back
     * ========================================================
     */
    SDL_FRect backBounds{
        bounds.x + bounds.w - 120.0f,
        bounds.y + bounds.h - 55.0f,
        90.0f,
        35.0f
    };

    drawText(
        renderer,
        optionFont_,
        "Back",
        backBounds.x + 20.0f,
        backBounds.y + 5.0f,
        TEXT_COLOR);

    SDL_SetRenderClipRect(
        renderer,
        nullptr);
}

// ============================================================
// Alarm editor
// ============================================================

void SettingsRenderer::renderAlarmEditor(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds,
    const AlarmConfig& alarm)
{
    SDL_SetRenderClipRect(
        renderer,
        nullptr);

    /*
     * Title.
     */
    drawText(
        renderer,
        titleFont_,
        "Alarm " +
            std::to_string(selectedAlarm_ + 1),
        bounds.x + 25.0f,
        bounds.y + 15.0f,
        TEXT_COLOR);

    /*
     * ========================================================
     * Alarm enabled
     * ========================================================
     */
    const float enabledY =
        bounds.y + 65.0f;

    drawText(
        renderer,
        optionFont_,
        "Alarm",
        bounds.x + 40.0f,
        enabledY,
        TEXT_COLOR);

    SDL_FRect enabledToggle{
        bounds.x + bounds.w - 105.0f,
        enabledY - 4.0f,
        70.0f,
        32.0f
    };

    drawToggle(
        renderer,
        enabledToggle,
        alarm.enabled);

    /*
     * ========================================================
     * Time
     * ========================================================
     */
    const float timeY =
        bounds.y + 110.0f;

    drawText(
        renderer,
        optionFont_,
        "Time",
        bounds.x + 40.0f,
        timeY,
        TEXT_COLOR);

    const bool isPM =
        alarm.hour >= 12;

    int displayHour =
        alarm.hour % 12;

    if (displayHour == 0)
    {
        displayHour = 12;
    }

    char hourText[8];

    std::snprintf(
        hourText,
        sizeof(hourText),
        "%d",
        displayHour);

    char minuteText[8];

    std::snprintf(
        minuteText,
        sizeof(minuteText),
        "%02d",
        alarm.minute);

    drawText(
        renderer,
        optionFont_,
        hourText,
        bounds.x + 160.0f,
        timeY,
        TEXT_COLOR);

    drawText(
        renderer,
        optionFont_,
        ":",
        bounds.x + 190.0f,
        timeY,
        TEXT_COLOR);

    drawText(
        renderer,
        optionFont_,
        minuteText,
        bounds.x + 205.0f,
        timeY,
        TEXT_COLOR);

    drawText(
        renderer,
        optionFont_,
        isPM ? "PM" : "AM",
        bounds.x + 260.0f,
        timeY,
        TEXT_COLOR);

    drawText(
        renderer,
        smallFont_,
        "tap hour / minute / AM-PM",
        bounds.x + 160.0f,
        timeY + 25.0f,
        MUTED_COLOR);

    /*
     * ========================================================
     * Repeat
     * ========================================================
     */
    const float repeatY =
        bounds.y + 165.0f;

    constexpr char DAY_NAMES[] =
    {
        'S',
        'M',
        'T',
        'W',
        'T',
        'F',
        'S'
    };

    constexpr float DAY_SIZE = 36.0f;
    constexpr float DAY_GAP = 4.0f;

    const float dayY =
        repeatY + 30.0f;

    for (int day = 0; day < 7; ++day)
    {
        SDL_FRect dayBounds{
            bounds.x + 40.0f +
                day * (DAY_SIZE + DAY_GAP),
            dayY,
            DAY_SIZE,
            DAY_SIZE
        };

        if (alarm.repeatDays[day])
        {
            SDL_SetRenderDrawColor(
                renderer,
                170,
                220,
                180,
                255);
        }
        else
        {
            SDL_SetRenderDrawColor(
                renderer,
                210,
                210,
                210,
                255);
        }

        SDL_RenderFillRect(
            renderer,
            &dayBounds);

        drawText(
            renderer,
            smallFont_,
            std::string(
                1,
                DAY_NAMES[day]),
            dayBounds.x + 12.0f,
            dayBounds.y + 8.0f,
            TEXT_COLOR);
    }

    /*
     * ========================================================
     * Sound selection
     * ========================================================
     */
    const float soundY =
        dayY + 50.0f;

    drawText(
        renderer,
        optionFont_,
        "Sound",
        bounds.x + 40.0f,
        soundY,
        TEXT_COLOR);

    drawText(
        renderer,
        smallFont_,
        getFileName(alarm.soundPath),
        bounds.x + 160.0f,
        soundY + 3.0f,
        MUTED_COLOR);

    drawText(
        renderer,
        optionFont_,
        ">",
        bounds.x + bounds.w - 45.0f,
        soundY,
        TEXT_COLOR);

    /*
     * ========================================================
     * Animation selection
     * ========================================================
     *
     * There is no longer an ON/OFF toggle.
     *
     * Empty animationDirectory = None.
     */
    const float animationSelectY =
        soundY + 42.0f;

    drawText(
        renderer,
        optionFont_,
        "Animation",
        bounds.x + 40.0f,
        animationSelectY,
        TEXT_COLOR);

    const std::string animationName =
        alarm.animationDirectory.empty()
            ? "None"
            : getFileName(
                  alarm.animationDirectory);

    drawText(
        renderer,
        smallFont_,
        animationName,
        bounds.x + 160.0f,
        animationSelectY + 3.0f,
        MUTED_COLOR);

    drawText(
        renderer,
        optionFont_,
        ">",
        bounds.x + bounds.w - 45.0f,
        animationSelectY,
        TEXT_COLOR);

    /*
     * ========================================================
     * Delete
     * ========================================================
     */
    SDL_FRect deleteBounds{
        bounds.x + 25.0f,
        bounds.y + bounds.h - 55.0f,
        90.0f,
        35.0f
    };

    drawText(
        renderer,
        optionFont_,
        "Delete",
        deleteBounds.x + 10.0f,
        deleteBounds.y + 5.0f,
        TEXT_COLOR);

    /*
     * ========================================================
     * Back
     * ========================================================
     */
    SDL_FRect backBounds{
        bounds.x + bounds.w - 120.0f,
        bounds.y + bounds.h - 55.0f,
        90.0f,
        35.0f
    };

    drawText(
        renderer,
        optionFont_,
        "Back",
        backBounds.x + 20.0f,
        backBounds.y + 5.0f,
        TEXT_COLOR);

    SDL_SetRenderClipRect(
        renderer,
        nullptr);
}

// ============================================================
// Alarm sound selector
// ============================================================

void SettingsRenderer::renderAlarmSoundSelector(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds,
    const AlarmConfig& alarm)
{
    SDL_SetRenderClipRect(
        renderer,
        nullptr);

    drawText(
        renderer,
        titleFont_,
        "Alarm Sound",
        bounds.x + 25.0f,
        bounds.y + 20.0f,
        TEXT_COLOR);

    const std::vector<std::string> sounds =
        getAlarmSounds();

    if (sounds.empty())
    {
        drawText(
            renderer,
            optionFont_,
            "No WAV files found",
            bounds.x + 40.0f,
            bounds.y + 90.0f,
            MUTED_COLOR);
    }
    else
    {
        float y =
            bounds.y + 75.0f;

        constexpr float ROW_HEIGHT = 48.0f;

        for (const std::string& sound :
             sounds)
        {
            const bool selected =
                sound == alarm.soundPath;

            SDL_FRect rowBounds{
                bounds.x + 25.0f,
                y - 5.0f,
                bounds.w - 50.0f,
                42.0f
            };

            if (selected)
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    170,
                    220,
                    180,
                    255);

                SDL_RenderFillRect(
                    renderer,
                    &rowBounds);
            }

            drawText(
                renderer,
                optionFont_,
                getFileName(sound),
                bounds.x + 40.0f,
                y + 3.0f,
                TEXT_COLOR);

            if (selected)
            {
                drawText(
                    renderer,
                    optionFont_,
                    "✓",
                    bounds.x + bounds.w - 65.0f,
                    y + 3.0f,
                    TEXT_COLOR);
            }

            y += ROW_HEIGHT;
        }
    }

    /*
     * Back.
     */
    SDL_FRect backBounds{
        bounds.x + bounds.w - 120.0f,
        bounds.y + bounds.h - 55.0f,
        90.0f,
        35.0f
    };

    drawText(
        renderer,
        optionFont_,
        "Back",
        backBounds.x + 20.0f,
        backBounds.y + 5.0f,
        TEXT_COLOR);

    SDL_SetRenderClipRect(
        renderer,
        nullptr);
}

// ============================================================
// Alarm animation selector
// ============================================================

void SettingsRenderer::renderAlarmAnimationSelector(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds,
    const AlarmConfig& alarm)
{
    SDL_SetRenderClipRect(
        renderer,
        nullptr);

    drawText(
        renderer,
        titleFont_,
        "Alarm Animation",
        bounds.x + 25.0f,
        bounds.y + 20.0f,
        TEXT_COLOR);

    const std::vector<std::string> animations =
        getAlarmAnimations();

    /*
     * ========================================================
     * None
     * ========================================================
     */
    float y =
        bounds.y + 75.0f;

    {
        const bool selected =
            alarm.animationDirectory.empty();

        SDL_FRect rowBounds{
            bounds.x + 25.0f,
            y - 5.0f,
            bounds.w - 50.0f,
            42.0f
        };

        if (selected)
        {
            SDL_SetRenderDrawColor(
                renderer,
                170,
                220,
                180,
                255);

            SDL_RenderFillRect(
                renderer,
                &rowBounds);
        }

        drawText(
            renderer,
            optionFont_,
            "None",
            bounds.x + 40.0f,
            y + 3.0f,
            TEXT_COLOR);

        if (selected)
        {
            drawText(
                renderer,
                optionFont_,
                "✓",
                bounds.x + bounds.w - 65.0f,
                y + 3.0f,
                TEXT_COLOR);
        }
    }

    y += 48.0f;

    /*
     * ========================================================
     * Animation folders
     * ========================================================
     */
    for (const std::string& animation :
         animations)
    {
        const bool selected =
            animation ==
            alarm.animationDirectory;

        SDL_FRect rowBounds{
            bounds.x + 25.0f,
            y - 5.0f,
            bounds.w - 50.0f,
            42.0f
        };

        if (selected)
        {
            SDL_SetRenderDrawColor(
                renderer,
                170,
                220,
                180,
                255);

            SDL_RenderFillRect(
                renderer,
                &rowBounds);
        }

        drawText(
            renderer,
            optionFont_,
            getFileName(animation),
            bounds.x + 40.0f,
            y + 3.0f,
            TEXT_COLOR);

        if (selected)
        {
            drawText(
                renderer,
                optionFont_,
                "✓",
                bounds.x + bounds.w - 65.0f,
                y + 3.0f,
                TEXT_COLOR);
        }

        y += 48.0f;
    }

    /*
     * Back.
     */
    SDL_FRect backBounds{
        bounds.x + bounds.w - 120.0f,
        bounds.y + bounds.h - 55.0f,
        90.0f,
        35.0f
    };

    drawText(
        renderer,
        optionFont_,
        "Back",
        backBounds.x + 20.0f,
        backBounds.y + 5.0f,
        TEXT_COLOR);

    SDL_SetRenderClipRect(
        renderer,
        nullptr);
}

// ============================================================
// Draw text
// ============================================================

void SettingsRenderer::drawText(
    SDL_Renderer* renderer,
    TTF_Font* font,
    const std::string& text,
    float x,
    float y,
    SDL_Color color)
{
    if (!font)
    {
        return;
    }

    SDL_Surface* surface =
        TTF_RenderText_Blended(
            font,
            text.c_str(),
            0,
            color);

    if (!surface)
    {
        return;
    }

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            renderer,
            surface);

    if (!texture)
    {
        SDL_DestroySurface(surface);
        return;
    }

    SDL_FRect destination{
        x,
        y,
        static_cast<float>(surface->w),
        static_cast<float>(surface->h)
    };

    SDL_RenderTexture(
        renderer,
        texture,
        nullptr,
        &destination);

    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

// ============================================================
// Toggle
// ============================================================

void SettingsRenderer::drawToggle(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds,
    bool enabled)
{
    if (enabled)
    {
        SDL_SetRenderDrawColor(
            renderer,
            170,
            220,
            180,
            255);
    }
    else
    {
        SDL_SetRenderDrawColor(
            renderer,
            190,
            190,
            190,
            255);
    }

    SDL_RenderFillRect(
        renderer,
        &bounds);

    drawText(
        renderer,
        optionFont_,
        enabled ? "ON" : "OFF",
        bounds.x + 15.0f,
        bounds.y + 5.0f,
        TEXT_COLOR);
}

// ============================================================
// Point in rectangle
// ============================================================

bool SettingsRenderer::pointInRect(
    float x,
    float y,
    const SDL_FRect& bounds) const
{
    return
        x >= bounds.x &&
        x <= bounds.x + bounds.w &&
        y >= bounds.y &&
        y <= bounds.y + bounds.h;
}

// ============================================================
// Get action
// ============================================================

SettingsAction SettingsRenderer::getAction(
    float x,
    float y,
    const SDL_FRect& bounds,
    const Settings& settings)
{
    const ClockSettings& config =
        settings.get();

    /*
     * ========================================================
     * Alarm sound selector
     * ========================================================
     */
    if (selectingAlarmSound_)
    {
        if (selectedAlarm_ >= config.alarms.size())
        {
            selectingAlarmSound_ = false;

            return SettingsAction::None;
        }

        const std::vector<std::string> sounds =
            getAlarmSounds();

        float soundY =
            bounds.y + 75.0f;

        constexpr float ROW_HEIGHT = 48.0f;

        for (const std::string& sound :
             sounds)
        {
            SDL_FRect soundBounds{
                bounds.x + 25.0f,
                soundY - 5.0f,
                bounds.w - 50.0f,
                42.0f
            };

            if (pointInRect(
                    x,
                    y,
                    soundBounds))
            {
                selectedAlarmSound_ =
                    sound;

                return SettingsAction::SelectAlarmSoundItem;
            }

            soundY += ROW_HEIGHT;
        }

        SDL_FRect backBounds{
            bounds.x + bounds.w - 120.0f,
            bounds.y + bounds.h - 55.0f,
            90.0f,
            35.0f
        };

        if (pointInRect(
                x,
                y,
                backBounds))
        {
            selectingAlarmSound_ = false;

            return SettingsAction::AlarmBack;
        }

        return SettingsAction::None;
    }

    /*
     * ========================================================
     * Alarm animation selector
     * ========================================================
     */
    if (selectingAlarmAnimation_)
    {
        if (selectedAlarm_ >= config.alarms.size())
        {
            selectingAlarmAnimation_ = false;

            return SettingsAction::None;
        }

        const std::vector<std::string> animations =
            getAlarmAnimations();

        /*
         * ----------------------------------------------------
         * None
         * ----------------------------------------------------
         */
        float animationY =
            bounds.y + 75.0f;

        SDL_FRect noneBounds{
            bounds.x + 25.0f,
            animationY - 5.0f,
            bounds.w - 50.0f,
            42.0f
        };

        if (pointInRect(
                x,
                y,
                noneBounds))
        {
            /*
             * Empty path represents "None".
             */
            selectedAlarmAnimation_.clear();

            return SettingsAction::SelectAlarmAnimationItem;
        }

        animationY += 48.0f;

        /*
         * ----------------------------------------------------
         * Animation folders
         * ----------------------------------------------------
         */
        constexpr float ROW_HEIGHT = 48.0f;

        for (const std::string& animation :
             animations)
        {
            SDL_FRect animationBounds{
                bounds.x + 25.0f,
                animationY - 5.0f,
                bounds.w - 50.0f,
                42.0f
            };

            if (pointInRect(
                    x,
                    y,
                    animationBounds))
            {
                selectedAlarmAnimation_ =
                    animation;

                return SettingsAction::SelectAlarmAnimationItem;
            }

            animationY += ROW_HEIGHT;
        }

        /*
         * Back.
         */
        SDL_FRect backBounds{
            bounds.x + bounds.w - 120.0f,
            bounds.y + bounds.h - 55.0f,
            90.0f,
            35.0f
        };

        if (pointInRect(
                x,
                y,
                backBounds))
        {
            selectingAlarmAnimation_ = false;

            return SettingsAction::AlarmAnimationBack;
        }

        return SettingsAction::None;
    }

    /*
     * ========================================================
     * Alarm editor
     * ========================================================
     */
    if (editingAlarm_)
    {
        if (selectedAlarm_ >= config.alarms.size())
        {
            editingAlarm_ = false;
            selectedAlarm_ = 0;

            return SettingsAction::None;
        }

        /*
         * Alarm enabled.
         */
        const float enabledY =
            bounds.y + 65.0f;

        SDL_FRect enabledToggle{
            bounds.x + bounds.w - 105.0f,
            enabledY - 4.0f,
            70.0f,
            32.0f
        };

        if (pointInRect(
                x,
                y,
                enabledToggle))
        {
            return SettingsAction::ToggleAlarmEnabled;
        }

        /*
         * Time.
         */
        const float timeY =
            bounds.y + 110.0f;

        SDL_FRect hourBounds{
            bounds.x + 150.0f,
            timeY - 5.0f,
            55.0f,
            40.0f
        };

        if (pointInRect(
                x,
                y,
                hourBounds))
        {
            return SettingsAction::AdjustAlarmHour;
        }

        SDL_FRect minuteBounds{
            bounds.x + 205.0f,
            timeY - 5.0f,
            55.0f,
            40.0f
        };

        if (pointInRect(
                x,
                y,
                minuteBounds))
        {
            return SettingsAction::AdjustAlarmMinute;
        }

        SDL_FRect amPmBounds{
            bounds.x + 255.0f,
            timeY - 5.0f,
            70.0f,
            40.0f
        };

        if (pointInRect(
                x,
                y,
                amPmBounds))
        {
            return SettingsAction::ToggleAlarmAmPm;
        }

        /*
         * Repeat days.
         */
        const float repeatY =
            bounds.y + 165.0f;

        const float dayY =
            repeatY + 30.0f;

        constexpr float DAY_SIZE = 36.0f;
        constexpr float DAY_GAP = 4.0f;

        for (int day = 0; day < 7; ++day)
        {
            SDL_FRect dayBounds{
                bounds.x + 40.0f +
                    day * (DAY_SIZE + DAY_GAP),
                dayY,
                DAY_SIZE,
                DAY_SIZE
            };

            if (pointInRect(
                    x,
                    y,
                    dayBounds))
            {
                selectedDay_ = day;

                return SettingsAction::ToggleAlarmDay;
            }
        }

        /*
         * Sound.
         */
        const float soundY =
            dayY + 50.0f;

        SDL_FRect soundBounds{
            bounds.x + 30.0f,
            soundY - 5.0f,
            bounds.w - 60.0f,
            40.0f
        };

        if (pointInRect(
                x,
                y,
                soundBounds))
        {
            selectedAlarmSound_ =
                config.alarms[selectedAlarm_].soundPath;

            selectingAlarmSound_ = true;

            return SettingsAction::SelectAlarmSound;
        }

        /*
         * ====================================================
         * Animation selection
         * ====================================================
         */
        const float animationSelectY =
            soundY + 42.0f;

        SDL_FRect animationSelectBounds{
            bounds.x + 30.0f,
            animationSelectY - 5.0f,
            bounds.w - 60.0f,
            40.0f
        };

        if (pointInRect(
                x,
                y,
                animationSelectBounds))
        {
            selectedAlarmAnimation_ =
                config.alarms[selectedAlarm_]
                    .animationDirectory;

            selectingAlarmAnimation_ = true;

            return SettingsAction::SelectAlarmAnimation;
        }

        /*
         * ====================================================
         * Delete
         * ====================================================
         */
        SDL_FRect deleteBounds{
            bounds.x + 25.0f,
            bounds.y + bounds.h - 55.0f,
            90.0f,
            35.0f
        };

        if (pointInRect(
                x,
                y,
                deleteBounds))
        {
            editingAlarm_ = false;
            selectingAlarmSound_ = false;
            selectingAlarmAnimation_ = false;

            selectedAlarmSound_.clear();
            selectedAlarmAnimation_.clear();

            alarmScrollOffset_ = 0.0f;

            return SettingsAction::DeleteAlarm;
        }

        /*
         * Back.
         */
        SDL_FRect backBounds{
            bounds.x + bounds.w - 120.0f,
            bounds.y + bounds.h - 55.0f,
            90.0f,
            35.0f
        };

        if (pointInRect(
                x,
                y,
                backBounds))
        {
            editingAlarm_ = false;

            return SettingsAction::AlarmBack;
        }

        return SettingsAction::None;
    }

    /*
     * ========================================================
     * Main Settings
     * ========================================================
     */

    const float scrollTop =
        bounds.y + 60.0f;

    const float scrollBottom =
        bounds.y + bounds.h - 65.0f;

    SDL_FRect scrollBounds{
        bounds.x,
        scrollTop,
        bounds.w,
        scrollBottom - scrollTop
    };

    const float firstRowY =
        bounds.y + 110.0f;

    const float songY =
        firstRowY + ROW_HEIGHT;

    const float alarmsTitleY =
        songY + 65.0f;

    const float alarmStartY =
        alarmsTitleY + 50.0f;

    const float addAlarmY =
        alarmStartY +
        static_cast<float>(config.alarms.size()) *
            ALARM_ROW_HEIGHT +
        10.0f;

    const float debugY =
        addAlarmY + 55.0f;

    const float contentBottom =
        debugY + 75.0f;

    const float contentTop =
        scrollTop;

    const float contentHeight =
        contentBottom - contentTop;

    const float maxScroll =
        std::max(
            0.0f,
            contentHeight - scrollBounds.h);

    alarmScrollOffset_ =
        std::clamp(
            alarmScrollOffset_,
            0.0f,
            maxScroll);

    const float scrollOffset =
        alarmScrollOffset_;

    /*
     * Music loop.
     */
    const float visibleFirstRowY =
        firstRowY - scrollOffset;

    SDL_FRect loopToggle{
        bounds.x + bounds.w - 105.0f,
        visibleFirstRowY - 4.0f,
        70.0f,
        32.0f
    };

    if (pointInRect(
            x,
            y,
            loopToggle) &&
        pointInRect(
            x,
            y,
            scrollBounds))
    {
        return SettingsAction::ToggleMusicLoop;
    }

    /*
     * Music Box Song.
     */
    const float visibleSongY =
        songY - scrollOffset;

    SDL_FRect songBounds{
        bounds.x + 30.0f,
        visibleSongY - 5.0f,
        bounds.w - 60.0f,
        50.0f
    };

    if (pointInRect(
            x,
            y,
            songBounds) &&
        pointInRect(
            x,
            y,
            scrollBounds))
    {
        return SettingsAction::SelectMusicSong;
    }

    /*
     * Alarm rows.
     */
    const float visibleAlarmStartY =
        alarmStartY - scrollOffset;

    if (pointInRect(
            x,
            y,
            scrollBounds))
    {
        for (std::size_t i = 0;
             i < config.alarms.size();
             ++i)
        {
            const float alarmY =
                visibleAlarmStartY +
                static_cast<float>(i) *
                    ALARM_ROW_HEIGHT;

            SDL_FRect alarmBounds{
                bounds.x + 30.0f,
                alarmY - 5.0f,
                bounds.w - 60.0f,
                55.0f
            };

            if (pointInRect(
                    x,
                    y,
                    alarmBounds))
            {
                selectedAlarm_ = i;
                editingAlarm_ = true;
                selectedDay_ = -1;

                return SettingsAction::SelectAlarm;
            }
        }

        /*
         * Add Alarm.
         */
        const float visibleAddAlarmY =
            addAlarmY - scrollOffset;

        SDL_FRect addAlarmBounds{
            bounds.x + 30.0f,
            visibleAddAlarmY,
            160.0f,
            40.0f
        };

        if (pointInRect(
                x,
                y,
                addAlarmBounds))
        {
            return SettingsAction::AddAlarm;
        }
    }

   /*
    * ========================================================
    * Debug
    * ========================================================
    */

    const float visibleDebugY =
        debugY - scrollOffset;

    SDL_FRect debugAnimationBounds{
        bounds.x + 30.0f,
        visibleDebugY + 25.0f,
        bounds.w - 60.0f,
        50.0f
    };

    if (pointInRect(
            x,
            y,
            debugAnimationBounds) &&
        pointInRect(
            x,
            y,
            scrollBounds))
    {
        return SettingsAction::DebugShowAnimation;
    }

    /*
     * Fixed Back.
     */
    SDL_FRect backBounds{
        bounds.x + bounds.w - 120.0f,
        bounds.y + bounds.h - 55.0f,
        90.0f,
        35.0f
    };

    if (pointInRect(
            x,
            y,
            backBounds))
    {
        return SettingsAction::Back;
    }

    return SettingsAction::None;
}

// ============================================================
// Scroll
// ============================================================

void SettingsRenderer::scrollAlarms(
    float amount)
{
    alarmScrollOffset_ += amount;
}

// ============================================================
// Selected alarm
// ============================================================

std::size_t SettingsRenderer::getSelectedAlarm() const
{
    return selectedAlarm_;
}

// ============================================================
// Alarm sound selection
// ============================================================

void SettingsRenderer::finishAlarmSoundSelection()
{
    selectingAlarmSound_ = false;
}

bool SettingsRenderer::isSelectingAlarmSound() const
{
    return selectingAlarmSound_;
}

std::string SettingsRenderer::getSelectedAlarmSound() const
{
    return selectedAlarmSound_;
}

// ============================================================
// Alarm animation selection
// ============================================================

void SettingsRenderer::finishAlarmAnimationSelection()
{
    selectingAlarmAnimation_ = false;
}

bool SettingsRenderer::isSelectingAlarmAnimation() const
{
    return selectingAlarmAnimation_;
}

std::string SettingsRenderer::getSelectedAlarmAnimation() const
{
    return selectedAlarmAnimation_;
}

// ============================================================
// Selected day
// ============================================================

int SettingsRenderer::getSelectedDay() const
{
    return selectedDay_;
}