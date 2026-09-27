#include "SceneRenderer.hpp"

#include "AppLayout.hpp"

#include <SDL3_image/SDL_image.h>

#include <cmath>
#include <iostream>

SceneRenderer::~SceneRenderer()
{
    destroy();
}

bool SceneRenderer::initialize(
    SDL_Renderer* renderer,
    const char* fontPath)
{
    renderer_ = renderer;

    if (!renderer_)
        return false;

    // ========================================================
    // Music Box font
    // ========================================================

    musicBoxFont_ =
        TTF_OpenFont(
            fontPath,
            20.0f);

    if (!musicBoxFont_)
    {
        std::cerr
            << "Failed to load Music Box font: "
            << SDL_GetError()
            << '\n';

        return false;
    }

    // ========================================================
    // Pochacco + bed
    // ========================================================

    pochaccoTexture_ =
        IMG_LoadTexture(
            renderer_,
            AppLayout::POCHACCO_PATH);

    if (!pochaccoTexture_)
    {
        std::cerr
            << "Failed to load "
            << AppLayout::POCHACCO_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        return false;
    }

    // ========================================================
    // Background textures
    // ========================================================

    morningTexture_ =
        IMG_LoadTexture(
            renderer_,
            AppLayout::MORNING_PATH);

    if (!morningTexture_)
    {
        std::cerr
            << "Failed to load "
            << AppLayout::MORNING_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        return false;
    }

    daytimeTexture_ =
        IMG_LoadTexture(
            renderer_,
            AppLayout::DAYTIME_PATH);

    if (!daytimeTexture_)
    {
        std::cerr
            << "Failed to load "
            << AppLayout::DAYTIME_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        return false;
    }

    eveningTexture_ =
        IMG_LoadTexture(
            renderer_,
            AppLayout::EVENING_PATH);

    if (!eveningTexture_)
    {
        std::cerr
            << "Failed to load "
            << AppLayout::EVENING_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        return false;
    }

    nightTexture_ =
        IMG_LoadTexture(
            renderer_,
            AppLayout::NIGHT_PATH);

    if (!nightTexture_)
    {
        std::cerr
            << "Failed to load "
            << AppLayout::NIGHT_PATH
            << ": "
            << SDL_GetError()
            << '\n';

        return false;
    }

    return true;
}

void SceneRenderer::destroy()
{
    if (nightTexture_)
    {
        SDL_DestroyTexture(nightTexture_);
        nightTexture_ = nullptr;
    }

    if (eveningTexture_)
    {
        SDL_DestroyTexture(eveningTexture_);
        eveningTexture_ = nullptr;
    }

    if (daytimeTexture_)
    {
        SDL_DestroyTexture(daytimeTexture_);
        daytimeTexture_ = nullptr;
    }

    if (morningTexture_)
    {
        SDL_DestroyTexture(morningTexture_);
        morningTexture_ = nullptr;
    }

    if (pochaccoTexture_)
    {
        SDL_DestroyTexture(pochaccoTexture_);
        pochaccoTexture_ = nullptr;
    }

    if (musicBoxFont_)
    {
        TTF_CloseFont(musicBoxFont_);
        musicBoxFont_ = nullptr;
    }
}

TimeOfDay SceneRenderer::getTimeOfDay(int hour)
{
    if (hour >= 6 && hour < 10)
        return TimeOfDay::Morning;

    if (hour >= 10 && hour < 17)
        return TimeOfDay::Day;

    if (hour >= 17 && hour < 20)
        return TimeOfDay::Evening;

    return TimeOfDay::Night;
}

SDL_Texture* SceneRenderer::getBackgroundTexture(
    TimeOfDay timeOfDay) const
{
    switch (timeOfDay)
    {
        case TimeOfDay::Morning:
            return morningTexture_;

        case TimeOfDay::Day:
            return daytimeTexture_;

        case TimeOfDay::Evening:
            return eveningTexture_;

        case TimeOfDay::Night:
            return nightTexture_;
    }

    return daytimeTexture_;
}

void SceneRenderer::update(float deltaTime)
{
    pochaccoAnimationTime_ += deltaTime;
}

void SceneRenderer::render(
    AppMode currentMode,
    const ClockTime& currentTime,
    const Settings& settings,
    ClockManager& clockManager,
    MusicManager& musicManager,
    SettingsRenderer& settingsRenderer)
{
    const TimeOfDay timeOfDay =
        getTimeOfDay(currentTime.hour);

    renderBackground(
        currentMode,
        timeOfDay,
        clockManager);

    switch (currentMode)
    {
        case AppMode::Clock:
            renderClock(
                currentTime,
                timeOfDay,
                clockManager,
                musicManager);
            break;

        case AppMode::Music:
            renderMusic(musicManager);
            break;

        case AppMode::Settings:
            renderSettings(
                settingsRenderer,
                settings);
            break;
    }
}

void SceneRenderer::renderBackground(
    AppMode currentMode,
    TimeOfDay timeOfDay,
    ClockManager& clockManager)
{
    if (currentMode == AppMode::Clock &&
        clockManager.isBirthdayActive())
    {
        clockManager.renderBirthdayBackground();
        return;
    }

    SDL_Texture* background = getBackgroundTexture(timeOfDay);

    if (!background)
        return;

    const SDL_FRect bounds = AppLayout::fullScreenBounds();

    SDL_RenderTexture(
        renderer_,
        background,
        nullptr,
        &bounds
    );
}

void SceneRenderer::renderClock(
    const ClockTime& currentTime,
    TimeOfDay timeOfDay,
    ClockManager& clockManager,
    MusicManager& musicManager)
{
    const bool birthday =
        clockManager.isBirthdayActive();

    const SDL_FRect clockBounds =
        AppLayout::clockBounds(birthday);

    const SDL_FRect musicBoxButtonBounds =
        AppLayout::musicBoxButtonBounds(birthday);

    clockManager.render(
        currentTime,
        clockBounds,
        timeOfDay);

    drawMusicBoxButton(
        musicManager.isMusicBoxPlaying(),
        musicBoxButtonBounds);
}

void SceneRenderer::renderMusic(
    MusicManager& musicManager)
{
    musicManager.render(
        renderer_,
        AppLayout::MUSIC_BOUNDS);
}

void SceneRenderer::renderSettings(
    SettingsRenderer& settingsRenderer,
    const Settings& settings)
{
    settingsRenderer.render(
        renderer_,
        AppLayout::fullScreenBounds(),
        settings);
}

void SceneRenderer::drawMusicBoxButton(
    bool playing,
    const SDL_FRect& bounds)
{
    if (!renderer_ || !musicBoxFont_)
        return;

    const SDL_Color buttonColor =
        playing
            ? AppLayout::MUSIC_BOX_BUTTON_ACTIVE_COLOR
            : AppLayout::MUSIC_BOX_BUTTON_COLOR;

    SDL_SetRenderDrawColor(
        renderer_,
        buttonColor.r,
        buttonColor.g,
        buttonColor.b,
        buttonColor.a);

    SDL_RenderFillRect(
        renderer_,
        &bounds);

    const char* text =
        playing
            ? "Music Box: ON"
            : "Music Box";

    SDL_Surface* surface =
        TTF_RenderText_Blended(
            musicBoxFont_,
            text,
            0,
            AppLayout::MUSIC_BOX_TEXT_COLOR);

    if (!surface)
        return;

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            renderer_,
            surface);

    if (!texture)
    {
        SDL_DestroySurface(surface);
        return;
    }

    SDL_FRect textBounds{
        bounds.x +
            (
                bounds.w -
                static_cast<float>(surface->w)
            ) / 2.0f,

        bounds.y +
            (
                bounds.h -
                static_cast<float>(surface->h)
            ) / 2.0f,

        static_cast<float>(surface->w),
        static_cast<float>(surface->h)
    };

    SDL_RenderTexture(
        renderer_,
        texture,
        nullptr,
        &textBounds);

    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

void SceneRenderer::shutdown()
{
    destroy();
}