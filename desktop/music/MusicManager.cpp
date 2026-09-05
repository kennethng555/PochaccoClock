#include "MusicManager.hpp"

bool MusicManager::initialize(
    SDL_Renderer* sdlRenderer)
{
    if (sdlRenderer == nullptr)
        return false;

    if (!renderer.initialize(sdlRenderer, "../assets/fonts/PinkyBlues.otf"))
    {
        SDL_Log("Failed to initialize MusicRenderer");
        return false;
    }

    initialized = true;

    return true;
}

void MusicManager::update()
{
    if (!initialized)
        return;

    renderer.update();
}

void MusicManager::render(
    SDL_Renderer* sdlRenderer,
    const SDL_FRect& bounds)
{
    if (!initialized)
        return;

    renderer.render(
        sdlRenderer,
        bounds);
}

void MusicManager::handleTouch(
    float x,
    float y,
    const SDL_FRect& bounds)
{
    if (!initialized)
        return;

    renderer.handleTouch(x, y, bounds);
}

void MusicManager::handleMouseClick(
    float x,
    float y,
    const SDL_FRect& bounds)
{
    if (!initialized)
        return;

    renderer.handleMouseClick(x, y, bounds);
}

void MusicManager::togglePlayPause()
{
    if (!initialized)
        return;

    renderer.togglePlayPause();
}

bool MusicManager::isInitialized() const
{
    return initialized;
}