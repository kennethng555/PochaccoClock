#include "Settings.hpp"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

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
    * Birthday
    * --------------------------------------------------------
    */

    config_.birthday.enabled = false;
    config_.birthday.month = 1;
    config_.birthday.day = 1;
    config_.birthday.name = "Pochacco";

    config_.birthday.animationDirectory =
        "../assets/animations/Birthday";

    config_.birthday.frameCount = 20;

    config_.birthday.musicPath =
        "../assets/music/HappyBirthday.wav";

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

    defaultAlarm.repeatDays = {
        true,
        true,
        true,
        true,
        true,
        false,
        false
    };

    defaultAlarm.playMusic = true;
    defaultAlarm.soundPath = "../assets/alarm/alarm.wav";
    defaultAlarm.animationDirectory = "";
    defaultAlarm.triggeredToday = false;

    config_.alarms.push_back(defaultAlarm);

    /*
     * --------------------------------------------------------
     * Animations
     * --------------------------------------------------------
     */

    /*
     * Simba
     */
    config_.animations[0].enabled = true;
    config_.animations[0].randomEnabled = false;

    config_.animations[0].scheduled = true;
    config_.animations[0].scheduledHour = 12;
    config_.animations[0].scheduledMinute = 0;

    config_.animations[0].name = "Simba";
    config_.animations[0].directory = "../assets/animations/Simba";
    config_.animations[0].frameCount = 8;

    config_.animations[0].frameDuration = 0.10f;
    config_.animations[0].displayDuration = 30.0f;

    config_.animations[0].startHour = 8;
    config_.animations[0].endHour = 22;

    config_.animations[0].x = 50.0f;
    config_.animations[0].y = 325.0f;
    config_.animations[0].width = 150.0f;
    config_.animations[0].height = 112.5f;

    /*
     * Toto
     */
    config_.animations[1].enabled = true;
    config_.animations[1].randomEnabled = false;

    config_.animations[1].scheduled = true;
    config_.animations[1].scheduledHour = 3;
    config_.animations[1].scheduledMinute = 0;

    config_.animations[1].name = "Toto";
    config_.animations[1].directory = "../assets/animations/Toto";
    config_.animations[1].frameCount = 6;

    config_.animations[1].frameDuration = 0.10f;
    config_.animations[1].displayDuration = 30.0f;

    config_.animations[1].startHour = 8;
    config_.animations[1].endHour = 22;

    config_.animations[1].x = 50.0f;
    config_.animations[1].y = 250.0f;
    config_.animations[1].width = 122.5f;
    config_.animations[1].height = 200.0f;

    /*
     * Remaining animation slots keep AnimationConfig defaults.
     */

    /*
     * --------------------------------------------------------
     * Music
     * --------------------------------------------------------
     */

    config_.music.songPath = "../assets/music/NextToYou.wav";
    config_.music.loop = true;
}

bool Settings::load(
    const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        return false;
    }

    try
    {
        json root;
        file >> root;

        /*
        * ----------------------------------------------------
        * Birthday
        * ----------------------------------------------------
        */

        if (root.contains("birthday")) {
            const json& birthday = root["birthday"];

            if (birthday.contains("enabled")) {
                config_.birthday.enabled =
                    birthday["enabled"].get<bool>();
            }

            if (birthday.contains("month")) {
                config_.birthday.month =
                    birthday["month"].get<int>();
            }

            if (birthday.contains("day")) {
                config_.birthday.day =
                    birthday["day"].get<int>();
            }

            if (birthday.contains("name")) {
                config_.birthday.name =
                    birthday["name"].get<std::string>();
            }

            if (birthday.contains("animationDirectory")) {
                config_.birthday.animationDirectory =
                    birthday["animationDirectory"].get<std::string>();
            }

            if (birthday.contains("frameCount")) {
                config_.birthday.frameCount =
                    birthday["frameCount"].get<std::size_t>();
            }

            if (birthday.contains("musicPath")) {
                config_.birthday.musicPath =
                    birthday["musicPath"].get<std::string>();
            }
        }

        /*
         * ----------------------------------------------------
         * Music
         * ----------------------------------------------------
         */

        if (root.contains("music")) {
            const json& music = root["music"];

            if (music.contains("songPath")) {
                config_.music.songPath = music["songPath"].get<std::string>();
            }

            if (music.contains("loop")) {
                config_.music.loop = music["loop"].get<bool>();
            }
        }

        /*
         * ----------------------------------------------------
         * Alarms
         * ----------------------------------------------------
         */

        if (root.contains("alarms") && root["alarms"].is_array()) {
            config_.alarms.clear();

            for (const json& item : root["alarms"]) {
                AlarmConfig alarm;

                if (item.contains("enabled")) {
                    alarm.enabled = item["enabled"].get<bool>();
                }

                if (item.contains("hour")) {
                    alarm.hour = item["hour"].get<int>();
                }

                if (item.contains("minute")) {
                    alarm.minute = item["minute"].get<int>();
                }

                if (item.contains("repeatDays") && item["repeatDays"].is_array())
                {
                    const json& days = item["repeatDays"];

                    for (std::size_t i = 0; i < 7 && i < days.size(); ++i) {
                        alarm.repeatDays[i] = days[i].get<bool>();
                    }
                }

                if (item.contains("playMusic")) {
                    alarm.playMusic = item["playMusic"].get<bool>();
                }

                if (item.contains("soundPath")) {
                    alarm.soundPath = item["soundPath"].get<std::string>();
                }

                if (item.contains("animationDirectory")) {
                    alarm.animationDirectory = item["animationDirectory"].get<std::string>();
                }
                else
                {
                    alarm.animationDirectory = "";
                }

                /*
                 * Runtime state must always start false.
                 */
                alarm.triggeredToday = false;

                config_.alarms.push_back(alarm);
            }
        }

        /*
         * ----------------------------------------------------
         * Animations
         * ----------------------------------------------------
         */

        if (root.contains("animations") && root["animations"].is_array()) {
            const json& animations = root["animations"];

            for (std::size_t i = 0; i < config_.animations.size() && i < animations.size(); ++i) {
                const json& item = animations[i];

                AnimationConfig& animation = config_.animations[i];

                if (item.contains("enabled")) {
                    animation.enabled = item["enabled"].get<bool>();
                }

                if (item.contains("randomEnabled")) {
                    animation.randomEnabled = item["randomEnabled"].get<bool>();
                }

                if (item.contains("scheduled")) {
                    animation.scheduled = item["scheduled"].get<bool>();
                }

                if (item.contains("scheduledHour")) {
                    animation.scheduledHour = item["scheduledHour"].get<int>();
                }

                if (item.contains("scheduledMinute")) {
                    animation.scheduledMinute = item["scheduledMinute"].get<int>();
                }

                if (item.contains("name")) {
                    animation.name = item["name"].get<std::string>();
                }

                if (item.contains("directory")) {
                    animation.directory = item["directory"].get<std::string>();
                }

                if (item.contains("frameCount")) {
                    animation.frameCount = item["frameCount"].get<std::size_t>();
                }

                if (item.contains("frameDuration")) {
                    animation.frameDuration = item["frameDuration"].get<float>();
                }

                if (item.contains("displayDuration")) {
                    animation.displayDuration = item["displayDuration"].get<float>();
                }

                if (item.contains("startHour")) {
                    animation.startHour = item["startHour"].get<int>();
                }

                if (item.contains("endHour")) {
                    animation.endHour = item["endHour"].get<int>();
                }

                /*
                 * Animation bounds
                 */

                if (item.contains("x")) {
                    animation.x = item["x"].get<float>();
                }

                if (item.contains("y")) {
                    animation.y = item["y"].get<float>();
                }

                if (item.contains("width")) {
                    animation.width = item["width"].get<float>();
                }

                if (item.contains("height")) {
                    animation.height = item["height"].get<float>();
                }
            }
        }

        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Settings: failed to parse " << path << ": " << e.what() << '\n';
        return false;
    }
}

bool Settings::save(
    const std::string& path) const
{
    try
    {
        json root;

        /*
         * ----------------------------------------------------
         * Music
         * ----------------------------------------------------
         */

        root["music"] = {
            {"songPath", config_.music.songPath},
            {"loop", config_.music.loop}
        };

        /*
         * ----------------------------------------------------
         * Alarms
         * ----------------------------------------------------
         */

        root["alarms"] = json::array();

        for (const AlarmConfig& alarm :
             config_.alarms)
        {
            json days =
                json::array();

            for (bool day :
                 alarm.repeatDays)
            {
                days.push_back(day);
            }

            root["alarms"].push_back({
                {"enabled", alarm.enabled},
                {"hour", alarm.hour},
                {"minute", alarm.minute},
                {"repeatDays", days},
                {"playMusic", alarm.playMusic},
                {"soundPath", alarm.soundPath},
                {"animationDirectory", alarm.animationDirectory}
            });

            /*
             * triggeredToday intentionally omitted.
             */
        }

        /*
        * ----------------------------------------------------
        * Birthday
        * ----------------------------------------------------
        */

        root["birthday"] = {
            {"enabled", config_.birthday.enabled},
            {"month", config_.birthday.month},
            {"day", config_.birthday.day},
            {"name", config_.birthday.name},
            {"animationDirectory",
            config_.birthday.animationDirectory},
            {"frameCount",
            config_.birthday.frameCount},
            {"musicPath",
            config_.birthday.musicPath}
        };

        /*
         * ----------------------------------------------------
         * Animations
         * ----------------------------------------------------
         */

        root["animations"] =
            json::array();

        for (const AnimationConfig& animation :
             config_.animations)
        {
            root["animations"].push_back({
                {"enabled", animation.enabled},
                {"randomEnabled", animation.randomEnabled},
                {"scheduled", animation.scheduled},
                {"scheduledHour", animation.scheduledHour},
                {"scheduledMinute", animation.scheduledMinute},

                {"name", animation.name},
                {"directory", animation.directory},
                {"frameCount", animation.frameCount},

                {"frameDuration",
                 animation.frameDuration},

                {"displayDuration",
                 animation.displayDuration},

                {"startHour", animation.startHour},
                {"endHour", animation.endHour},

                /*
                 * Animation bounds
                 */
                {"x", animation.x},
                {"y", animation.y},
                {"width", animation.width},
                {"height", animation.height}
            });
        }

        /*
         * ----------------------------------------------------
         * Write JSON
         * ----------------------------------------------------
         */

        std::ofstream file(path);

        if (!file.is_open()) {
            std::cerr << "Settings: failed to open " << path << " for writing\n";
            return false;
        }

        file << root.dump(4) << '\n';

        return true;
    } catch (const json::exception& e) {
        std::cerr << "Settings: failed to save " << path << ": " << e.what() << '\n';
        return false;
    }
}