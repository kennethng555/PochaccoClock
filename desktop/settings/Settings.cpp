#include "Settings.hpp"

#include <fstream>

Settings::Settings()
{
    createDefaults();
}

ClockSettings& Settings::get()
{
    return config_;
}

const ClockSettings& Settings::get() const
{
    return config_;
}

void Settings::createDefaults()
{
    /*
     * --------------------------------------------------------
     * Alarms
     * --------------------------------------------------------
     */

    config_.alarms.clear();

    AlarmConfig defaultAlarm;

    defaultAlarm.enabled = false;
    defaultAlarm.hour = 7;
    defaultAlarm.minute = 0;

    config_.alarms.push_back(
        defaultAlarm);

    /*
     * --------------------------------------------------------
     * Animations
     * --------------------------------------------------------
     */

    config_.animations[0].enabled = true;
    config_.animations[0].randomEnabled = false;

    config_.animations[0].scheduled = true;
    config_.animations[0].scheduledHour = 12;
    config_.animations[0].scheduledMinute = 0;

    config_.animations[0].name = "Simba";
    config_.animations[0].directory = "../assets/Simba";
    config_.animations[0].frameCount = 8;

    config_.animations[0].displayDuration = 30.0f;

    config_.animations[0].x = 5.0f;
    config_.animations[0].y = 325.0f;
    config_.animations[0].width = 150.0f;
    config_.animations[0].height = 112.5f;

    // config_.animations[1].name = "Pochacco";
    // config_.animations[1].directory =
    //     "../assets/Pochacco";
    // config_.animations[1].frameCount = 16;

    // config_.animations[2].name = "Animation 3";
    // config_.animations[2].directory =
    //     "../assets/Animation3";
    // config_.animations[2].frameCount = 16;

    // config_.animations[3].name = "Animation 4";
    // config_.animations[3].directory =
    //     "../assets/Animation4";
    // config_.animations[3].frameCount = 16;

    /*
     * --------------------------------------------------------
     * Music
     * --------------------------------------------------------
     */

    config_.music.enabled = false;

    config_.music.songPath =
        "../assets/musicbox/NextToYou.wav";

    config_.music.loop = true;
}

bool Settings::load(
    const std::string& path)
{
    /*
     * Persistence can be implemented next.
     *
     * For now, retain defaults if the file does
     * not exist.
     */
    std::ifstream file(path);

    if (!file.is_open())
    {
        return false;
    }

    /*
     * We'll add the actual configuration format once
     * the settings UI is working.
     */
    return true;
}

bool Settings::save(
    const std::string& path) const
{
    std::ofstream file(path);

    if (!file.is_open())
    {
        return false;
    }

    /*
     * Persistence implementation will be added here.
     */
    return true;
}