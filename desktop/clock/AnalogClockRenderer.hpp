#pragma once

#include <SDL3/SDL.h>

#include "IClockRenderer.hpp"
#include "ClockTime.hpp"

class AnalogClockRenderer : public IClockRenderer
{
public:
    explicit AnalogClockRenderer(
        SDL_Renderer* renderer);

    void render(
        const ClockTime& time,
        const SDL_FRect& bounds,
        float alpha = 1.0f) override;

private:
    void drawClockFace(
        float centerX,
        float centerY,
        float radius,
        Uint8 alpha);

    void drawMinuteMarkers(
        float centerX,
        float centerY,
        float radius,
        Uint8 alpha);

    void drawHourMarkers(
        float centerX,
        float centerY,
        float radius,
        Uint8 alpha);

    void drawHands(
        const ClockTime& time,
        float centerX,
        float centerY,
        float radius,
        Uint8 alpha);

    void drawHand(
        double angle,
        float length,
        float width,
        float centerX,
        float centerY,
        Uint8 alpha);

    void drawFilledCircle(
        float centerX,
        float centerY,
        float radius,
        Uint8 alpha);

private:
    SDL_Renderer* renderer_;
};