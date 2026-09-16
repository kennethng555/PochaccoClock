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
    SelectAlarmSound,
    SelectAlarmSoundItem,
    SelectAlarmAnimation,
    SelectAlarmAnimationItem,
    AlarmAnimationBack,
    ToggleAlarmDay,
    DeleteAlarm,
    DebugShowAnimation,

    // Navigation
    Back,
    AlarmBack
};