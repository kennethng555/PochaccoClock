#ifndef CLOCK_MANAGER_HPP
#define CLOCK_MANAGER_HPP

#include "DigitalClockRenderer.hpp"
#include "../animation/AnimationManager.hpp"

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

    bool initialize();

    void update(float deltaTime);

    void render(
        const ClockTime& time,
        const SDL_FRect& bounds,
        TimeOfDay timeOfDay);

private:
    SDL_Renderer* renderer_;

    std::unique_ptr<DigitalClockRenderer>
        digitalRenderer_;

    AnimationManager animationManager_;

    float elapsedTime_;
};

#endif