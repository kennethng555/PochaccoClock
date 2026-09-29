#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>

#include "ClockTime.hpp"
#include "TimeOfDay.hpp"

class DigitalClockRenderer
{
public:

    DigitalClockRenderer(
        SDL_Renderer* renderer,
        const std::string& fontPath
    );

    ~DigitalClockRenderer();

    bool initialize();

    void render(
        const ClockTime& time,
        const SDL_FRect& bounds,
        TimeOfDay timeOfDay
    );

    void setBirthdayMode(bool enabled);

private:

    void drawTime(
        const ClockTime& time,
        const SDL_FRect& bounds,
        SDL_Color textColor,
        SDL_Color outlineColor);

    void drawAmPm(
        const ClockTime& time,
        const SDL_FRect& bounds,
        SDL_Color textColor,
        SDL_Color outlineColor);

    void drawDate(
        const ClockTime& time,
        const SDL_FRect& bounds,
        SDL_Color textColor,
        SDL_Color outlineColor);

    void drawText(
        TTF_Font* font,
        const char* text,
        SDL_Color color,
        float x,
        float y,
        bool centered = true
    );

private:

    SDL_Renderer* renderer_;

    std::string fontPath_;

    TTF_Font* timeFont_;
    TTF_Font* smallFont_;
    
    bool birthdayMode_ = false;
};