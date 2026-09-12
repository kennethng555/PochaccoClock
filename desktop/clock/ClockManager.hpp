#pragma once

#include "DigitalClockRenderer.hpp"
#include "../animation/AnimationManager.hpp"
#include "../settings/Settings.hpp"

#include <SDL3/SDL.h>

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

private:
    SDL_Renderer* renderer_;

    std::unique_ptr<DigitalClockRenderer>
        digitalRenderer_;

    AnimationManager animationManager_;

    float elapsedTime_;
};