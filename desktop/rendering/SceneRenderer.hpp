#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "../app/AppMode.hpp"
#include "../clock/ClockManager.hpp"
#include "../clock/ClockTime.hpp"
#include "../clock/TimeOfDay.hpp"
#include "../music/MusicManager.hpp"
#include "../settings/Settings.hpp"
#include "../settings/SettingsRenderer.hpp"

class SceneRenderer
{
public:
    SceneRenderer() = default;
    ~SceneRenderer();

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    bool initialize(
        SDL_Renderer* renderer,
        const char* fontPath);

    void update(float deltaTime);

    void render(
        AppMode currentMode,
        const ClockTime& currentTime,
        const Settings& settings,
        ClockManager& clockManager,
        MusicManager& musicManager,
        SettingsRenderer& settingsRenderer);

    void shutdown();

    static TimeOfDay getTimeOfDay(int hour);

private:
    void destroy();

    SDL_Texture* getBackgroundTexture(
        TimeOfDay timeOfDay) const;

    void renderBackground(
        AppMode currentMode,
        TimeOfDay timeOfDay,
        ClockManager& clockManager);

    void renderPochacco();

    void renderClock(
        const ClockTime& currentTime,
        TimeOfDay timeOfDay,
        ClockManager& clockManager,
        MusicManager& musicManager);

    void renderMusic(
        MusicManager& musicManager);

    void renderSettings(
        SettingsRenderer& settingsRenderer,
        const Settings& settings);

    void drawMusicBoxButton(
        bool playing,
        const SDL_FRect& bounds);

private:
    SDL_Renderer* renderer_ = nullptr;

    TTF_Font* musicBoxFont_ = nullptr;

    SDL_Texture* pochaccoTexture_ = nullptr;

    SDL_Texture* morningTexture_ = nullptr;
    SDL_Texture* daytimeTexture_ = nullptr;
    SDL_Texture* eveningTexture_ = nullptr;
    SDL_Texture* nightTexture_ = nullptr;

    float pochaccoAnimationTime_ = 0.0f;
};