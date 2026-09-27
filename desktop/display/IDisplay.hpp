#pragma once

#include <SDL3/SDL.h>

class IDisplay
{
public:
    virtual ~IDisplay() = default;

    virtual bool initialize(
        const char* title,
        int width,
        int height) = 0;

    virtual void beginFrame(
        SDL_Color background) = 0;

    virtual void endFrame() = 0;

    virtual bool processEvents(
        bool& running) = 0;

    virtual SDL_Renderer* renderer() = 0;

    virtual SDL_Window* window() = 0;

    virtual void shutdown() = 0;
};