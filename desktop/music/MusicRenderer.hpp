#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <string>

#include "MusicPlayer.hpp"
#include "SpectrumAnalyzer.hpp"

enum class MusicAction
    {
        None,
        PlayPause,
        Previous,
        Next,
        ToggleLoop,
        ToggleShuffle
    };


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
    
    MusicAction getAction(
        float x,
        float y,
        const SDL_FRect& bounds) const;
    
    bool isPlayButtonClicked(
        float x,
        float y,
        const SDL_FRect& bounds) const;

    void togglePlayPause();

    MusicPlayer& getMusicPlayer();
    const MusicPlayer& getMusicPlayer() const;

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

    void renderLoopButton(
        SDL_Renderer* renderer,
        float centerX,
        float centerY);

    void renderPreviousButton(
        SDL_Renderer* renderer,
        float centerX,
        float centerY);

    void renderPlayButton(
        SDL_Renderer* renderer,
        float centerX,
        float centerY);

    void renderNextButton(
        SDL_Renderer* renderer,
        float centerX,
        float centerY);

    void renderShuffleButton(
        SDL_Renderer* renderer,
        float centerX,
        float centerY);
    
    void renderActiveButtonCircle(
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
    // Spectrum
    // ========================================================

    static constexpr std::size_t ANALYZER_SIZE = 512;

    // ========================================================
    // Colors
    // ========================================================

    SDL_Color titleColor{
        70,
        100,
        70,
        255
    };

    SDL_Color artistColor{
        110,
        140,
        110,
        255
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
        100,
        125,
        100,
        255
    };

    SDL_Color iconColor{
        255,
        255,
        255,
        255
    };
};