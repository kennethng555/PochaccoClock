#include "AnimatedImage.hpp"

#include <cstdio>

AnimatedImage::~AnimatedImage()
{
    for (SDL_Texture* texture : frames)
    {
        if (texture != nullptr)
        {
            SDL_DestroyTexture(texture);
        }
    }

    frames.clear();
}

bool AnimatedImage::load(
    SDL_Renderer* renderer,
    const std::string& directory,
    std::size_t frameCount)
{
    if (renderer == nullptr)
        return false;

    if (frameCount == 0)
        return false;

    /*
     * Clear any previously loaded animation.
     */
    for (SDL_Texture* texture : frames)
    {
        if (texture != nullptr)
        {
            SDL_DestroyTexture(texture);
        }
    }

    frames.clear();

    currentFrame = 0;
    frameTimer = 0.0f;

    /*
     * Load every PNG frame.
     */
    for (std::size_t i = 0;
         i < frameCount;
         ++i)
    {
        char filename[64];

        std::snprintf(
            filename,
            sizeof(filename),
            "/frame_%02zu.png",
            i);

        const std::string path =
            directory + filename;

        SDL_Surface* surface =
            IMG_Load(path.c_str());

        if (surface == nullptr)
        {
            SDL_Log(
                "AnimatedImage: failed to load '%s': %s",
                path.c_str(),
                SDL_GetError());

            /*
             * Clean up frames already loaded.
             */
            for (SDL_Texture* texture : frames)
            {
                SDL_DestroyTexture(texture);
            }

            frames.clear();

            return false;
        }

        SDL_Texture* texture =
            SDL_CreateTextureFromSurface(
                renderer,
                surface);

        SDL_DestroySurface(surface);

        if (texture == nullptr)
        {
            SDL_Log(
                "AnimatedImage: failed to create texture for '%s': %s",
                path.c_str(),
                SDL_GetError());

            for (SDL_Texture* existing : frames)
            {
                SDL_DestroyTexture(existing);
            }

            frames.clear();

            return false;
        }

        /*
         * Enable alpha transparency.
         */
        SDL_SetTextureBlendMode(
            texture,
            SDL_BLENDMODE_BLEND);

        frames.push_back(texture);
    }

    return !frames.empty();
}

void AnimatedImage::update(
    float deltaTime)
{
    if (frames.empty())
        return;

    if (deltaTime <= 0.0f)
        return;

    frameTimer += deltaTime;

    /*
     * Advance as many frames as necessary.
     *
     * Using a while loop prevents the animation from
     * becoming permanently slow if one update takes
     * longer than a frame.
     */
    while (frameTimer >= frameDuration)
    {
        frameTimer -= frameDuration;

        ++currentFrame;

        /*
         * Loop back to frame zero.
         */
        if (currentFrame >= frames.size())
        {
            currentFrame = 0;
        }
    }
}

void AnimatedImage::render(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds)
{
    if (renderer == nullptr)
        return;

    if (frames.empty())
        return;

    SDL_Texture* texture =
        frames[currentFrame];

    if (texture == nullptr)
        return;

    SDL_RenderTexture(
        renderer,
        texture,
        nullptr,
        &bounds);
}

void AnimatedImage::reset()
{
    currentFrame = 0;
    frameTimer = 0.0f;
}