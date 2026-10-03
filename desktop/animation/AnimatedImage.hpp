#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <cstddef>
#include <string>
#include <vector>
#include <utility>

class AnimatedImage
{
public:

    AnimatedImage() = default;

    ~AnimatedImage();

    AnimatedImage(
        const AnimatedImage&) = delete;

    AnimatedImage& operator=(
        const AnimatedImage&) = delete;

    AnimatedImage(
        AnimatedImage&& other) noexcept;

    AnimatedImage& operator=(
        AnimatedImage&& other) noexcept;

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

    float frameDuration = 0.15f;
};