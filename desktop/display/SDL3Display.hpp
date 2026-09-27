#pragma once

#include "IDisplay.hpp"

class SDL3Display : public IDisplay
{
public:
    SDL3Display();
    ~SDL3Display() override;

    bool initialize(
        const char* title,
        int width,
        int height) override;

    virtual void beginFrame(
        SDL_Color background) override;

    void endFrame() override;

    bool processEvents(
        bool& running) override;

    SDL_Renderer* renderer() override;

    SDL_Window* window() override;

    void shutdown() override;

private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
};