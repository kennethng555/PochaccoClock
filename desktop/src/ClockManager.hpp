#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <string>

#include "ClockTime.hpp"
#include "DigitalClockRenderer.hpp"
#include "TimeOfDay.hpp"

class ClockManager
{
public:

    ClockManager(
        SDL_Renderer* renderer,
        const std::string& fontPath
    );

    ~ClockManager();

    bool initialize();

    void update(
        float deltaTime
    );

    void render(
        const ClockTime& time,
        const SDL_FRect& bounds,
        TimeOfDay timeOfDay
    );

private:

    std::unique_ptr<DigitalClockRenderer>
        digitalRenderer_;

    float elapsedTime_;
};