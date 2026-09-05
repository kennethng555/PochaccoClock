#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <string>

#include "MusicPlayer.hpp"
#include "SpectrumAnalyzer.hpp"

class MusicRenderer
{
public:
    MusicRenderer() = default;
    ~MusicRenderer();

    bool initialize(
        SDL_Renderer* renderer,
        const char* fontPath);

    void update();

    void render(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds);

    void handleTouch(
        float x,
        float y,
        const SDL_FRect& bounds);

    void handleMouseClick(
        float x,
        float y,
        const SDL_FRect& bounds);

    void togglePlayPause();

private:
    // ========================================================
    // Rendering
    // ========================================================

    void renderSpectrum(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds);

    void renderSongInfo(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds);

    void renderProgressBar(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds);

    void renderControls(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds);

    void renderPlayButton(
        SDL_Renderer* renderer,
        float centerX,
        float centerY,
        float radius);

    void renderPreviousButton(
        SDL_Renderer* renderer,
        float centerX,
        float centerY);

    void renderNextButton(
        SDL_Renderer* renderer,
        float centerX,
        float centerY);

    // ========================================================
    // Text
    // ========================================================

    void renderText(
        SDL_Renderer* renderer,
        const std::string& text,
        TTF_Font* font,
        SDL_Color color,
        float x,
        float y);

    void renderCenteredText(
        SDL_Renderer* renderer,
        const std::string& text,
        TTF_Font* font,
        SDL_Color color,
        float centerX,
        float y);

    // ========================================================
    // Input
    // ========================================================

    bool pointInRect(
        float x,
        float y,
        const SDL_FRect& rect) const;

    void seekFromPosition(
        float x,
        const SDL_FRect& progressBounds);

    // ========================================================
    // Utilities
    // ========================================================

    SDL_Texture* createTextTexture(
        SDL_Renderer* renderer,
        const std::string& text,
        TTF_Font* font,
        SDL_Color color);

    std::string formatTime(
        float seconds) const;

private:
    SDL_Renderer* renderer = nullptr;

    MusicPlayer musicPlayer;

    SpectrumAnalyzer spectrumAnalyzer;

    TTF_Font* titleFont = nullptr;
    TTF_Font* artistFont = nullptr;
    TTF_Font* smallFont = nullptr;

    bool initialized = false;

    // ========================================================
    // Animation
    // ========================================================

    float animationTime = 0.0f;

    // ========================================================
    // Song information
    // ========================================================

    std::string songTitle =
        "Pochacco Dreams";

    std::string artistName =
        "Pochacco";

    // ========================================================
    // Spectrum
    // ========================================================

    static constexpr std::size_t ANALYZER_SIZE = 512;

    // ========================================================
    // Colors
    // ========================================================

    SDL_Color titleColor{
        0, 0, 0, 255
    };

    SDL_Color artistColor{
        0, 0, 0, 255
    };

    SDL_Color primaryColor{
        145,
        190,
        145,
        255
    };

    SDL_Color secondaryColor{
        190,
        215,
        190,
        255
    };

    SDL_Color backgroundBarColor{
        220,
        230,
        220,
        255
    };

    SDL_Color timeColor{
        0, 0, 0, 255
    };

    SDL_Color iconColor{
        0, 0, 0, 255
    };
};