#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

struct AlarmConfig
{
    bool enabled = false;

    int hour = 7;
    int minute = 0;

    /*
     * Sunday = 0
     * Monday = 1
     * ...
     * Saturday = 6
     */
    std::array<bool, 7> repeatDays{
        true,
        true,
        true,
        true,
        true,
        false,
        false
    };

    bool playMusic = true;
    bool showAnimation = true;

    std::string soundPath =
        "../assets/alarm/alarm.wav";

    /*
     * Runtime state.
     * Do not persist this value.
     */
    bool triggeredToday = false;
};

struct AnimationConfig
{
    bool enabled = true;
    bool randomEnabled = false;

    bool scheduled = false;
    int scheduledHour = 0;
    int scheduledMinute = 0;

    std::string name;
    std::string directory;

    std::size_t frameCount = 0;

    float frameDuration = 0.10f;

    float displayDuration = 30.0f;

    /*
     * Allowed hours for random appearances.
     */
    int startHour = 8;
    int endHour = 22;

    /*
     * Location on the 600x450 display.
     */
    float x = 5.0f;
    float y = 325.0f;
    float width = 150.0f;
    float height = 112.5f;
};

struct MusicSettings
{
    std::string songPath;

    bool loop = true;
};

struct ClockSettings
{
    std::vector<AlarmConfig> alarms;

    std::array<AnimationConfig, 4> animations;

    MusicSettings music;
};

class Settings
{
public:
    Settings();

    ClockSettings& get();
    const ClockSettings& get() const;

    bool load(const std::string& path);
    bool save(const std::string& path) const;

private:
    ClockSettings config_;

    void createDefaults();
};