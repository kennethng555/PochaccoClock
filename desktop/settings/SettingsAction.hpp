#pragma once

enum class SettingsAction
{
    None,

    // Music
    ToggleMusicLoop,
    SelectMusicSong,

    // Alarm list
    ToggleAlarmEnabled,
    SelectAlarm,
    AddAlarm,

    // Alarm editor
    AdjustAlarmHour,
    AdjustAlarmMinute,
    ToggleAlarmAmPm,
    ToggleAlarmAnimation,
    SelectAlarmSound,
    SelectAlarmSoundItem,
    ToggleAlarmDay,
    DeleteAlarm,

    // Navigation
    Back,
    AlarmBack
};