#pragma once

#include <SDL3/SDL.h>

#include "ClockTime.hpp"

class IClockRenderer
{
public:
    virtual ~IClockRenderer() = default;

    virtual void render(
      const ClockTime& time,
      const SDL_FRect& bounds,
      float alpha = 1.0f) = 0;
};