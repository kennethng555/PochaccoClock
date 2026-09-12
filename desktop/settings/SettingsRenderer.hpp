#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <string>

#include "Settings.hpp"
#include "SettingsAction.hpp"

class SettingsRenderer
{
public:
    SettingsRenderer() = default;
    ~SettingsRenderer();

    bool initialize(
        SDL_Renderer* renderer,
        const std::string& fontPath);

    void render(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds,
        const Settings& settings);

    SettingsAction getAction(
        float x,
        float y,
        const SDL_FRect& bounds,
        const Settings& settings);

    std::size_t getSelectedAlarm() const;
    bool isSelectingAlarmSound() const;
    std::string getSelectedAlarmSound() const;
    void finishAlarmSoundSelection();
    void updateAlarmScroll(
      float delta);

    void handleAlarmScrollStart(float y);
    void handleAlarmScrollMove(float y);
    void handleAlarmScrollEnd();
    void scrollAlarms(float amount);

    int getSelectedDay() const;

private:
    void renderAlarmSoundSelector(
      SDL_Renderer* renderer,
      const SDL_FRect& bounds,
      const AlarmConfig& alarm);

    void drawText(
        SDL_Renderer* renderer,
        TTF_Font* font,
        const std::string& text,
        float x,
        float y,
        SDL_Color color);

    void drawToggle(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds,
        bool enabled);

    bool pointInRect(
        float x,
        float y,
        const SDL_FRect& bounds) const;

    void renderMainSettings(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds,
        const ClockSettings& config);

    void renderAlarmEditor(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds,
        const AlarmConfig& alarm);

private:
    TTF_Font* titleFont_ = nullptr;
    TTF_Font* optionFont_ = nullptr;
    TTF_Font* smallFont_ = nullptr;

    bool initialized_ = false;

    mutable bool editingAlarm_ = false;
    mutable std::size_t selectedAlarm_ = 0;
    mutable int selectedDay_ = -1;
    bool selectingAlarmSound_ = false;
    std::string selectedAlarmSound_;
    float alarmScrollOffset_ = 0.0f;

    bool alarmDragging_ = false;
    float alarmDragStartY_ = 0.0f;
    float alarmScrollStart_ = 0.0f;
};