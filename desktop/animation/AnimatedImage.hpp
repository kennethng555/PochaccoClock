#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <string>
#include <vector>

class AnimatedImage
{
public:

    ~AnimatedImage();

    bool load(
        SDL_Renderer* renderer,
        const std::string& directory,
        std::size_t frameCount);

    void update(
        float deltaTime);

    void render(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds);

    void reset();

private:

    std::vector<SDL_Texture*> frames;

    std::size_t currentFrame = 0;

    float frameTimer = 0.0f;

    /*
     * 200 ms per frame = 5 FPS.
     */
    float frameDuration = 0.20f;
};