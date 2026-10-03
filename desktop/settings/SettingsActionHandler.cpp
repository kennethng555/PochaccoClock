#include "SettingsActionHandler.hpp"

#include "SettingsRenderer.hpp"

#include "../clock/ClockManager.hpp"
#include "../music/MusicManager.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

namespace
{

constexpr const char* SETTINGS_PATH =
    "../assets/settings/settings.json";

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

} // namespace

void SettingsActionHandler::handle(
    SettingsAction action,
    Settings& settings,
    SettingsRenderer& settingsRenderer,
    MusicManager& musicManager,
    ClockManager& clockManager,
    AppMode& currentMode)
{
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

            newAlarm.soundPath =
                "../assets/alarm/alarm.wav";

            newAlarm.animationDirectory = "";

            settings.get().alarms.push_back(
                newAlarm);

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::ToggleAlarmEnabled:
        {
            auto& alarms =
                settings.get().alarms;

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
                    settingsRenderer
                        .getSelectedAlarmSound();
            }

            settingsRenderer
                .finishAlarmSoundSelection();

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::SelectAlarmAnimation:
            break;

        case SettingsAction::SelectAlarmAnimationItem:
        {
            auto& alarms =
                settings.get().alarms;

            const std::size_t index =
                settingsRenderer.getSelectedAlarm();

            if (index < alarms.size())
            {
                alarms[index].animationDirectory =
                    settingsRenderer
                        .getSelectedAlarmAnimation();
            }

            settingsRenderer
                .finishAlarmAnimationSelection();

            settings.save(SETTINGS_PATH);

            break;
        }

        case SettingsAction::AlarmAnimationBack:
            break;

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

        case SettingsAction::DebugShowAnimation:
        {
            clockManager
                .getAnimationManager()
                .showNextAnimation();

            currentMode = AppMode::Clock;

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