
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

    if (!std::filesystem::exists(directory))
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
}

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

void SettingsRenderer::render(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds,
    const Settings& settings)
{
    if (!initialized_)
    {
        return;
    }

    /*
     * IMPORTANT:
     *
     * SDL renderer state persists between frames.
     *
     * Always begin the settings screen with
     * clipping disabled so a clip left behind by
     * another rendering operation cannot blank the
     * settings screen.
     */
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
    if (editingAlarm_ &&
        selectedAlarm_ >= config.alarms.size())
    {
        editingAlarm_ = false;
        selectedAlarm_ = 0;
    }

    /*
     * Alarm sound selector.
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

        /*
         * Do not leave clipping enabled.
         */
        SDL_SetRenderClipRect(
            renderer,
            nullptr);

        return;
    }

    /*
     * Alarm editor.
     */
    if (editingAlarm_)
    {
        renderAlarmEditor(
            renderer,
            bounds,
            config.alarms[selectedAlarm_]);

        /*
         * Do not leave clipping enabled.
         */
        SDL_SetRenderClipRect(
            renderer,
            nullptr);

        return;
    }

    /*
     * Main settings.
     */
    renderMainSettings(
        renderer,
        bounds,
        config);

    /*
     * Defensive cleanup:
     * the settings renderer should never leave
     * a clip rectangle enabled.
     */
    SDL_SetRenderClipRect(
        renderer,
        nullptr);
}

void SettingsRenderer::renderMainSettings(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds,
    const ClockSettings& config)
{
    /*
     * Make absolutely sure the main settings page
     * begins without a clip rectangle.
     */
    SDL_SetRenderClipRect(
        renderer,
        nullptr);

    /*
     * Title.
     */
    drawText(
        renderer,
        titleFont_,
        "Settings",
        bounds.x + 25.0f,
        bounds.y + 20.0f,
        TEXT_COLOR);

    /*
     * -------------------------
     * Music
     * -------------------------
     */
    drawText(
        renderer,
        optionFont_,
        "Music",
        bounds.x + 30.0f,
        bounds.y + 70.0f,
        TEXT_COLOR);

    const float firstRowY =
        bounds.y + 110.0f;

    /*
     * Loop Music.
     */
    drawText(
        renderer,
        optionFont_,
        "MusicBox Loop",
        bounds.x + 40.0f,
        firstRowY,
        TEXT_COLOR);

    SDL_FRect loopToggle{
        bounds.x + bounds.w - 105.0f,
        firstRowY - 4.0f,
        70.0f,
        32.0f
    };

    drawToggle(
        renderer,
        loopToggle,
        config.music.loop);

    /*
     * Music Box Song.
     */
    const float songY =
        firstRowY + ROW_HEIGHT;

    drawText(
        renderer,
        optionFont_,
        "Music Box Song",
        bounds.x + 40.0f,
        songY,
        TEXT_COLOR);

    drawText(
        renderer,
        smallFont_,
        getFileName(config.music.songPath),
        bounds.x + 40.0f,
        songY + 27.0f,
        MUTED_COLOR);

    drawText(
        renderer,
        optionFont_,
        ">",
        bounds.x + bounds.w - 45.0f,
        songY + 5.0f,
        TEXT_COLOR);

    /*
     * -------------------------
     * Alarms
     * -------------------------
     */
    const float alarmsTitleY =
        songY + 65.0f;

    drawText(
        renderer,
        optionFont_,
        "Alarms",
        bounds.x + 30.0f,
        alarmsTitleY,
        TEXT_COLOR);

    /*
     * -------------------------
     * Alarm list bounds
     * -------------------------
     *
     * This rectangle is ONLY the viewport
     * for the scrolling alarm list.
     */
    SDL_FRect alarmListBounds{
        bounds.x,
        alarmsTitleY + 50.0f,
        bounds.w,
        (
            bounds.y +
            bounds.h -
            85.0f
        ) -
        (
            alarmsTitleY + 50.0f
        )
    };

    /*
     * Total alarm content height.
     */
    const float contentHeight =
        static_cast<float>(config.alarms.size()) *
        ALARM_ROW_HEIGHT;

    /*
     * Maximum scroll distance.
     */
    const float maxScroll =
        std::max(
            0.0f,
            contentHeight -
                alarmListBounds.h);

    /*
     * Clamp scroll position.
     */
    alarmScrollOffset_ =
        std::clamp(
            alarmScrollOffset_,
            0.0f,
            maxScroll);

    /*
     * Convert the alarm viewport to an SDL_Rect.
     */
    SDL_Rect alarmClip{
        static_cast<int>(alarmListBounds.x),
        static_cast<int>(alarmListBounds.y),
        static_cast<int>(alarmListBounds.w),
        static_cast<int>(alarmListBounds.h)
    };

    /*
     * Enable clipping ONLY for the alarm list.
     */
    SDL_SetRenderClipRect(
        renderer,
        &alarmClip);

    /*
     * Alarm content starts at the top of the
     * alarm list and moves upward as the user
     * scrolls.
     */
    const float alarmStartY =
        alarmListBounds.y -
        alarmScrollOffset_;

    for (std::size_t i = 0;
         i < config.alarms.size();
         ++i)
    {
        const float alarmY =
            alarmStartY +
            static_cast<float>(i) *
                ALARM_ROW_HEIGHT;

        const AlarmConfig& alarm =
            config.alarms[i];

        /*
         * Alarm name.
         */
        drawText(
            renderer,
            optionFont_,
            "Alarm " +
                std::to_string(i + 1),
            bounds.x + 40.0f,
            alarmY,
            TEXT_COLOR);

        /*
         * Alarm time.
         */
        drawText(
            renderer,
            smallFont_,
            formatAlarmTime(
                alarm.hour,
                alarm.minute),
            bounds.x + 40.0f,
            alarmY + 26.0f,
            MUTED_COLOR);

        /*
         * ON/OFF.
         */
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

        /*
         * Arrow.
         */
        drawText(
            renderer,
            optionFont_,
            ">",
            bounds.x + bounds.w - 45.0f,
            alarmY + 5.0f,
            TEXT_COLOR);
    }

    /*
     * IMPORTANT:
     *
     * Do NOT restore an SDL_Rect captured from
     * SDL_GetRenderClipRect().
     *
     * If there was no previous clip rectangle,
     * the correct state is simply "no clip".
     */
    SDL_SetRenderClipRect(
        renderer,
        nullptr);

    /*
     * -------------------------
     * Add Alarm
     * -------------------------
     */
    SDL_FRect addAlarmBounds{
        bounds.x + 30.0f,
        bounds.y + bounds.h - 85.0f,
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
     * -------------------------
     * Back
     * -------------------------
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

    /*
     * Final defensive cleanup.
     */
    SDL_SetRenderClipRect(
        renderer,
        nullptr);
}

void SettingsRenderer::renderAlarmEditor(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds,
    const AlarmConfig& alarm)
{
    /*
     * Make sure the editor is never clipped by
     * the main alarm-list viewport.
     */
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
        bounds.y + 20.0f,
        TEXT_COLOR);

    /*
     * Enabled.
     */
    const float enabledY =
        bounds.y + 75.0f;

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
     * Time.
     */
    const float timeY =
        bounds.y + 125.0f;

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
        timeY + 27.0f,
        MUTED_COLOR);

    /*
     * Repeat.
     */
    const float repeatY =
        bounds.y + 185.0f;

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

    constexpr float DAY_SIZE = 38.0f;
    constexpr float DAY_GAP = 5.0f;

    const float dayY =
        repeatY + 35.0f;

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
            dayBounds.x + 13.0f,
            dayBounds.y + 9.0f,
            TEXT_COLOR);
    }

    /*
     * Sound.
     */
    const float soundY =
        dayY + 60.0f;

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
     * Animation.
     */
    const float animationY =
        soundY + 55.0f;

    drawText(
        renderer,
        optionFont_,
        "Animation",
        bounds.x + 40.0f,
        animationY,
        TEXT_COLOR);

    SDL_FRect animationToggle{
        bounds.x + bounds.w - 105.0f,
        animationY - 4.0f,
        70.0f,
        32.0f
    };

    drawToggle(
        renderer,
        animationToggle,
        alarm.showAnimation);

    /*
     * Delete.
     */
    const float deleteY =
        animationY + 55.0f;

    drawText(
        renderer,
        optionFont_,
        "Delete",
        bounds.x + 40.0f,
        deleteY,
        TEXT_COLOR);

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

    /*
     * Defensive cleanup.
     */
    SDL_SetRenderClipRect(
        renderer,
        nullptr);
}

void SettingsRenderer::renderAlarmSoundSelector(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds,
    const AlarmConfig& alarm)
{
    /*
     * Make sure the sound selector starts
     * without any inherited clipping.
     */
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

    /*
     * Defensive cleanup.
     */
    SDL_SetRenderClipRect(
        renderer,
        nullptr);
}

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

SettingsAction SettingsRenderer::getAction(
    float x,
    float y,
    const SDL_FRect& bounds,
    const Settings& settings)
{
    const ClockSettings& config =
        settings.get();

    /*
     * =========================
     * Alarm sound selector
     * =========================
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
     * =========================
     * Alarm editor
     * =========================
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
            bounds.y + 75.0f;

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
         * Hour.
         */
        const float timeY =
            bounds.y + 125.0f;

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

        /*
         * Minute.
         */
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

        /*
         * AM / PM.
         */
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
            bounds.y + 185.0f;

        const float dayY =
            repeatY + 35.0f;

        constexpr float DAY_SIZE = 38.0f;
        constexpr float DAY_GAP = 5.0f;

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
            dayY + 60.0f;

        SDL_FRect soundBounds{
            bounds.x + 30.0f,
            soundY - 5.0f,
            bounds.w - 60.0f,
            45.0f
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
         * Animation.
         */
        const float animationY =
            soundY + 55.0f;

        SDL_FRect animationToggle{
            bounds.x + bounds.w - 105.0f,
            animationY - 4.0f,
            70.0f,
            32.0f
        };

        if (pointInRect(
                x,
                y,
                animationToggle))
        {
            return SettingsAction::ToggleAlarmAnimation;
        }

        /*
        * Delete.
        */
        const float deleteY =
            animationY + 55.0f;

        SDL_FRect deleteBounds{
            bounds.x + 30.0f,
            deleteY - 5.0f,
            120.0f,
            40.0f
        };

        if (pointInRect(
                x,
                y,
                deleteBounds))
        {
            editingAlarm_ = false;
            selectingAlarmSound_ = false;
            selectedAlarmSound_.clear();
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
     * =========================
     * Main settings
     * =========================
     */

    /*
     * Loop Music.
     */
    const float firstRowY =
        bounds.y + 110.0f;

    SDL_FRect loopToggle{
        bounds.x + bounds.w - 105.0f,
        firstRowY - 4.0f,
        70.0f,
        32.0f
    };

    if (pointInRect(
            x,
            y,
            loopToggle))
    {
        return SettingsAction::ToggleMusicLoop;
    }

    /*
     * Music Box Song.
     */
    const float songY =
        firstRowY + ROW_HEIGHT;

    SDL_FRect songBounds{
        bounds.x + 30.0f,
        songY - 5.0f,
        bounds.w - 60.0f,
        50.0f
    };

    if (pointInRect(
            x,
            y,
            songBounds))
    {
        return SettingsAction::SelectMusicSong;
    }

    /*
     * -------------------------
     * Alarm list bounds
     * -------------------------
     */
    const float alarmsTitleY =
        songY + 65.0f;

    SDL_FRect alarmListBounds{
        bounds.x,
        alarmsTitleY + 50.0f,
        bounds.w,
        (
            bounds.y +
            bounds.h -
            85.0f
        ) -
        (
            alarmsTitleY + 50.0f
        )
    };

    /*
     * Calculate scroll limits from
     * the actual alarm list rectangle.
     */
    const float contentHeight =
        static_cast<float>(config.alarms.size()) *
        ALARM_ROW_HEIGHT;

    const float maxScroll =
        std::max(
            0.0f,
            contentHeight -
                alarmListBounds.h);

    alarmScrollOffset_ =
        std::clamp(
            alarmScrollOffset_,
            0.0f,
            maxScroll);

    /*
     * Only process alarm clicks inside
     * the alarm list rectangle.
     */
    if (pointInRect(
            x,
            y,
            alarmListBounds))
    {
        const float alarmStartY =
            alarmListBounds.y -
            alarmScrollOffset_;

        for (std::size_t i = 0;
             i < config.alarms.size();
             ++i)
        {
            const float alarmY =
                alarmStartY +
                static_cast<float>(i) *
                    ALARM_ROW_HEIGHT;

            SDL_FRect alarmBounds{
                alarmListBounds.x + 30.0f,
                alarmY - 5.0f,
                alarmListBounds.w - 60.0f,
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
    }

    /*
     * Add Alarm.
     */
    SDL_FRect addAlarmBounds{
        bounds.x + 30.0f,
        bounds.y + bounds.h - 85.0f,
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
        return SettingsAction::Back;
    }

    return SettingsAction::None;
}

void SettingsRenderer::scrollAlarms(
    float amount)
{
    alarmScrollOffset_ += amount;
}

std::size_t SettingsRenderer::getSelectedAlarm() const
{
    return selectedAlarm_;
}

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

int SettingsRenderer::getSelectedDay() const
{
    return selectedDay_;
}