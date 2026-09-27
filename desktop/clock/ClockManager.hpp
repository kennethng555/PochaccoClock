#pragma once

#include "DigitalClockRenderer.hpp"
#include "../animation/AnimationManager.hpp"
#include "../settings/Settings.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <memory>
#include <string>

class ClockManager
{
public:

    ClockManager(
        SDL_Renderer* renderer,
        const std::string& fontPath);

    ~ClockManager();

    bool initialize(
        const ClockSettings& settings);

    void update(
        float deltaTime,
        const ClockSettings& settings,
        const ClockTime& time);

    void render(
        const ClockTime& time,
        const SDL_FRect& bounds,
        TimeOfDay timeOfDay);

    AnimationManager& getAnimationManager();

    // ========================================================
    // Birthday
    // ========================================================

    void startBirthday(
        const BirthdaySettings& settings);

    void stopBirthday();

    void updateBirthday(
        float deltaTime);

    void renderBirthdayBackground();

    bool isBirthdayActive() const;

private:

    void renderBirthdayText();

    SDL_Renderer* renderer_;

    std::string fontPath_;

    std::unique_ptr<
        DigitalClockRenderer
    > digitalRenderer_;

    AnimationManager animationManager_;

    float elapsedTime_;

    // ========================================================
    // Birthday
    // ========================================================

    bool birthdayActive_ = false;

    BirthdaySettings birthdaySettings_{};

    TTF_Font* birthdayFont_ = nullptr;

    TTF_Font* birthdayNameFont_ = nullptr;
};