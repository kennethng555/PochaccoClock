#pragma once

#include "../app/AppMode.hpp"
#include "SettingsAction.hpp"
#include "Settings.hpp"

class SettingsRenderer;
class MusicManager;
class ClockManager;

class SettingsActionHandler
{
public:
    static void handle(
        SettingsAction action,
        Settings& settings,
        SettingsRenderer& settingsRenderer,
        MusicManager& musicManager,
        ClockManager& clockManager,
        AppMode& currentMode);
};