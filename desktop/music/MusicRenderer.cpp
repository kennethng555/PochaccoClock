#include "MusicRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{
    // ========================================================
    // Layout
    // ========================================================

    constexpr float SPECTRUM_HEIGHT = 155.0f;
    constexpr float SONG_INFO_HEIGHT = 58.0f;
    constexpr float PROGRESS_HEIGHT = 45.0f;
    constexpr float CONTROLS_HEIGHT = 75.0f;
    constexpr float PROGRESS_BAR_HEIGHT = 7.0f;
    constexpr float CONTROL_SPACING = 78.0f;
    constexpr float PLAY_BUTTON_RADIUS = 27.0f;
    constexpr float SIDE_BUTTON_SIZE = 52.0f;
    constexpr float MUSIC_PANEL_X = 0.0f;
    constexpr float MUSIC_PANEL_Y = 0.0f;
    constexpr float MUSIC_PANEL_WIDTH = 560.0f;
    constexpr float MUSIC_PANEL_HEIGHT = 343.0f;

    // ========================================================
    // Font sizes
    // ========================================================

    constexpr int TITLE_FONT_SIZE = 28;
    constexpr int ARTIST_FONT_SIZE = 21;
    constexpr float SMALL_FONT_SIZE = 15.0f;

    // ========================================================
    // Spectrum
    // ========================================================

    constexpr std::size_t SPECTRUM_BARS = 32;

    // ========================================================
    // Animation
    // ========================================================

    constexpr float ANIMATION_SPEED = 2.0f;
    constexpr float PLAYING_PULSE_AMOUNT = 0.04f;
}

// ============================================================
// Destructor
// ============================================================

MusicRenderer::~MusicRenderer()
{
    /*
     * Fonts must be destroyed while SDL_ttf is still active.
     *
     * MusicRenderer itself is scoped inside main(), so this
     * destructor executes before TTF_Quit().
     */

    if (titleFont != nullptr)
    {
        TTF_CloseFont(titleFont);
        titleFont = nullptr;
    }

    if (artistFont != nullptr)
    {
        TTF_CloseFont(artistFont);
        artistFont = nullptr;
    }

    if (smallFont != nullptr)
    {
        TTF_CloseFont(smallFont);
        smallFont = nullptr;
    }
}

// ============================================================
// Initialize
// ============================================================

bool MusicRenderer::initialize(
    SDL_Renderer* sdlRenderer,
    const char* fontPath)
{
    if (sdlRenderer == nullptr)
        return false;

    if (fontPath == nullptr)
        return false;

    renderer = sdlRenderer;

    /*
     * Load fonts.
     */

    titleFont =
        TTF_OpenFont(
            fontPath,
            TITLE_FONT_SIZE);

    if (titleFont == nullptr)
    {
        SDL_Log(
            "MusicRenderer: failed to load title font: %s",
            SDL_GetError());

        return false;
    }

    artistFont =
        TTF_OpenFont(
            fontPath,
            ARTIST_FONT_SIZE);

    if (artistFont == nullptr)
    {
        SDL_Log(
            "MusicRenderer: failed to load artist font: %s",
            SDL_GetError());

        return false;
    }

    smallFont =
        TTF_OpenFont(
            fontPath,
            SMALL_FONT_SIZE);

    if (smallFont == nullptr)
    {
        SDL_Log(
            "MusicRenderer: failed to load small font: %s",
            SDL_GetError());

        return false;
    }

    /*
     * Initialize the music player.
     */

    if (!musicPlayer.initialize())
    {
        SDL_Log(
            "MusicRenderer: MusicPlayer initialization failed");

        return false;
    }

    /*
     * Temporary desktop track.
     *
     * This will eventually become the selected MP3/music
     * source when the audio backend is expanded.
     */
    if (!musicPlayer.loadDirectory("../assets/music"))
    {
        SDL_Log(
            "MusicRenderer: failed to load test music");

        return false;
    }

    musicPlayer.play();

    initialized = true;

    return true;
}

// ============================================================
// Update
// ============================================================

void MusicRenderer::update()
{
    if (!initialized)
        return;

    /*
     * Update actual audio playback.
     */
    musicPlayer.update();

    /*
     * Advance UI animation.
     *
     * This doesn't depend on audio playback.
     */
    animationTime +=
        1.0f / 60.0f;

    /*
     * Feed PCM samples to the spectrum analyzer.
     */

    const float* samples =
        musicPlayer.getSamples();

    const std::size_t sampleCount =
        musicPlayer.getSampleCount();

    if (samples == nullptr ||
        sampleCount == 0)
    {
        return;
    }

    /*
     * Determine the current sample position.
     *
     * The current implementation assumes 44.1 kHz.
     * The MusicPlayer abstraction can expose the actual
     * sample rate later when the MP3 backend is added.
     */
    constexpr float SAMPLE_RATE = 44100.0f;

    const std::size_t currentSample =
        static_cast<std::size_t>(
            musicPlayer.getCurrentTime() *
            SAMPLE_RATE);

    if (currentSample >= sampleCount)
        return;

    const std::size_t availableSamples =
        sampleCount -
        currentSample;

    const std::size_t samplesToAnalyze =
        std::min(
            availableSamples,
            ANALYZER_SIZE);

    if (samplesToAnalyze == 0)
        return;

    spectrumAnalyzer.update(
        samples + currentSample,
        samplesToAnalyze);
}

// ============================================================
// Main render
// ============================================================

void MusicRenderer::render(
    SDL_Renderer* sdlRenderer,
    const SDL_FRect& bounds)
{
    if (!initialized)
        return;

    if (sdlRenderer == nullptr)
        return;


    // ========================================================
    // Translucent music panel
    // ========================================================

    SDL_FRect panelBounds{
        bounds.x + MUSIC_PANEL_X,
        bounds.y + MUSIC_PANEL_Y,
        MUSIC_PANEL_WIDTH,
        MUSIC_PANEL_HEIGHT
    };

    SDL_SetRenderDrawBlendMode(
        sdlRenderer,
        SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(
        sdlRenderer,
        255,
        252,
        240,
        205);

    SDL_RenderFillRect(
        sdlRenderer,
        &panelBounds);

    /*
     * Divide the music area vertically.
     */

    float y =
        bounds.y;

    // --------------------------------------------------------
    // Spectrum
    // --------------------------------------------------------

    SDL_FRect spectrumBounds{
        bounds.x,
        y,
        bounds.w,
        SPECTRUM_HEIGHT
    };

    renderSpectrum(
        sdlRenderer,
        spectrumBounds);

    y += SPECTRUM_HEIGHT;

    // --------------------------------------------------------
    // Song information
    // --------------------------------------------------------

    SDL_FRect songInfoBounds{
        bounds.x,
        y,
        bounds.w,
        SONG_INFO_HEIGHT
    };

    renderSongInfo(
        sdlRenderer,
        songInfoBounds);

    y += SONG_INFO_HEIGHT;

    // --------------------------------------------------------
    // Progress
    // --------------------------------------------------------

    SDL_FRect progressBounds{
        bounds.x,
        y,
        bounds.w,
        PROGRESS_HEIGHT
    };

    renderProgressBar(
        sdlRenderer,
        progressBounds);

    y += PROGRESS_HEIGHT;

    // --------------------------------------------------------
    // Controls
    // --------------------------------------------------------

    SDL_FRect controlsBounds{
        bounds.x,
        y,
        bounds.w,
        CONTROLS_HEIGHT
    };

    renderControls(
        sdlRenderer,
        controlsBounds);
}

// ============================================================
// Spectrum
// ============================================================

void MusicRenderer::renderSpectrum(
    SDL_Renderer* sdlRenderer,
    const SDL_FRect& bounds)
{
    /*
     * Give the spectrum some internal padding.
     */

    constexpr float PADDING_X = 10.0f;
    constexpr float PADDING_Y = 10.0f;

    SDL_FRect analyzerBounds{
        bounds.x + PADDING_X,
        bounds.y + PADDING_Y,
        bounds.w - PADDING_X * 2.0f,
        bounds.h - PADDING_Y * 2.0f
    };

    /*
     * Let SpectrumAnalyzer render its bars.
     */
    spectrumAnalyzer.render(
        sdlRenderer,
        analyzerBounds);
}

// ============================================================
// Song information
// ============================================================

void MusicRenderer::renderSongInfo(
    SDL_Renderer* sdlRenderer,
    const SDL_FRect& bounds)
{
    /*
     * Left align title and artist.
     */

    constexpr float LEFT_PADDING = 10.0f;

    const char* title =
        musicPlayer.getTitle().empty()
            ? musicPlayer.getFilename().c_str()
            : musicPlayer.getTitle().c_str();

    const char* artist =
        musicPlayer.getArtist().empty()
            ? "Unknown Artist"
            : musicPlayer.getArtist().c_str();

    renderText(
        sdlRenderer,
        title,
        titleFont,
        titleColor,
        bounds.x + LEFT_PADDING,
        bounds.y + 2.0f);

    renderText(
        sdlRenderer,
        artist,
        artistFont,
        artistColor,
        bounds.x + LEFT_PADDING,
        bounds.y + 31.0f);
}

// ============================================================
// Progress bar
// ============================================================

void MusicRenderer::renderProgressBar(
    SDL_Renderer* sdlRenderer,
    const SDL_FRect& bounds)
{
    const float duration =
        musicPlayer.getDuration();

    const float currentTime =
        musicPlayer.getCurrentTime();

    float progress = 0.0f;

    if (duration > 0.0f)
    {
        progress =
            currentTime /
            duration;
    }

    progress =
        std::clamp(
            progress,
            0.0f,
            1.0f);

    constexpr float TIME_WIDTH = 45.0f;

    /*
     * Current time.
     */

    renderText(
        sdlRenderer,
        formatTime(currentTime),
        smallFont,
        timeColor,
        bounds.x,
        bounds.y + 1.0f);

    /*
     * Duration.
     */

    const std::string durationText =
        formatTime(duration);

    int durationTextWidth = 0;
    int durationTextHeight = 0;

    TTF_GetStringSize(
        smallFont,
        durationText.c_str(),
        0,
        &durationTextWidth,
        &durationTextHeight);

    renderText(
        sdlRenderer,
        durationText,
        smallFont,
        timeColor,
        bounds.x +
            bounds.w -
            static_cast<float>(durationTextWidth),
        bounds.y + 1.0f);

    /*
     * Progress bar sits between the time labels.
     */

    SDL_FRect barBounds{
        bounds.x + TIME_WIDTH,
        bounds.y + 10.0f,
        bounds.w - TIME_WIDTH * 2.0f,
        PROGRESS_BAR_HEIGHT
    };

    /*
     * Track.
     */

    SDL_SetRenderDrawColor(
        sdlRenderer,
        backgroundBarColor.r,
        backgroundBarColor.g,
        backgroundBarColor.b,
        backgroundBarColor.a);

    SDL_RenderFillRect(
        sdlRenderer,
        &barBounds);

    /*
     * Played portion.
     */

    SDL_FRect playedBounds =
        barBounds;

    playedBounds.w *=
        progress;

    SDL_SetRenderDrawColor(
        sdlRenderer,
        primaryColor.r,
        primaryColor.g,
        primaryColor.b,
        primaryColor.a);

    SDL_RenderFillRect(
        sdlRenderer,
        &playedBounds);

    /*
     * Position indicator.
     */

    const float knobRadius = 5.0f;

    const float knobX =
        barBounds.x +
        barBounds.w * progress;

    const float knobY =
        barBounds.y +
        barBounds.h / 2.0f;

    SDL_FRect knob{
        knobX - knobRadius,
        knobY - knobRadius,
        knobRadius * 2.0f,
        knobRadius * 2.0f
    };

    SDL_SetRenderDrawColor(
        sdlRenderer,
        primaryColor.r,
        primaryColor.g,
        primaryColor.b,
        primaryColor.a);

    SDL_RenderFillRect(
        sdlRenderer,
        &knob);
}

// ============================================================
// Controls
// ============================================================

void MusicRenderer::renderControls(
    SDL_Renderer* sdlRenderer,
    const SDL_FRect& bounds)
{
    const float centerX =
        bounds.x +
        bounds.w / 2.0f;

    const float centerY =
        bounds.y +
        bounds.h / 2.0f;

    /*
     * Previous
     */

    renderPreviousButton(
        sdlRenderer,
        centerX - CONTROL_SPACING,
        centerY);

    /*
     * Play / pause
     */

    renderPlayButton(
        sdlRenderer,
        centerX,
        centerY);

    /*
     * Next
     */

    renderNextButton(
        sdlRenderer,
        centerX + CONTROL_SPACING,
        centerY);
}

// ============================================================
// Play / pause button
// ============================================================

void MusicRenderer::renderPlayButton(
    SDL_Renderer* renderer,
    float centerX,
    float centerY)
{
    if (renderer == nullptr)
        return;

    constexpr float RADIUS = 27.0f;

    SDL_SetRenderDrawColor(
        renderer,
        170,
        220,
        170,
        255);

    /*
     * Draw a fixed-size circular play button.
     */
    for (int y = -static_cast<int>(RADIUS);
         y <= static_cast<int>(RADIUS);
         ++y)
    {
        for (int x = -static_cast<int>(RADIUS);
             x <= static_cast<int>(RADIUS);
             ++x)
        {
            if (x * x + y * y <=
                RADIUS * RADIUS)
            {
                SDL_RenderPoint(
                    renderer,
                    centerX + static_cast<float>(x),
                    centerY + static_cast<float>(y));
            }
        }
    }

    /*
     * Fixed play/pause icon.
     *
     * Play = right-facing triangle
     * Pause = two vertical bars
     */
    SDL_SetRenderDrawColor(
        renderer,
        0,
        0,
        0,
        255);

    if (musicPlayer.isPlaying())
    {
        constexpr float BAR_WIDTH = 6.0f;
        constexpr float BAR_HEIGHT = 20.0f;
        constexpr float BAR_GAP = 5.0f;

        SDL_FRect leftBar{
            centerX - BAR_GAP / 2.0f - BAR_WIDTH,
            centerY - BAR_HEIGHT / 2.0f,
            BAR_WIDTH,
            BAR_HEIGHT
        };

        SDL_FRect rightBar{
            centerX + BAR_GAP / 2.0f,
            centerY - BAR_HEIGHT / 2.0f,
            BAR_WIDTH,
            BAR_HEIGHT
        };

        SDL_RenderFillRect(
            renderer,
            &leftBar);

        SDL_RenderFillRect(
            renderer,
            &rightBar);
    }
    else
    {
        /*
         * Right-facing play triangle.
         */
        SDL_Vertex vertices[3]{};

        vertices[0].position = {
            centerX - 7.0f,
            centerY - 12.0f
        };

        vertices[1].position = {
            centerX - 7.0f,
            centerY + 12.0f
        };

        vertices[2].position = {
            centerX + 13.0f,
            centerY
        };

        for (auto& vertex : vertices)
        {
            vertex.color = {
                0,
                0,
                0,
                255
            };
        }

        SDL_RenderGeometry(
            renderer,
            nullptr,
            vertices,
            3,
            nullptr,
            0);
    }
}

// ============================================================
// Previous button
// ============================================================

void MusicRenderer::renderPreviousButton(
    SDL_Renderer* sdlRenderer,
    float centerX,
    float centerY)
{
    if (sdlRenderer == nullptr)
        return;

    SDL_SetRenderDrawColor(
        sdlRenderer,
        0,
        0,
        0,
        255);

    constexpr float BAR_WIDTH = 4.0f;
    constexpr float BAR_HEIGHT = 22.0f;

    /*
     * Previous = |◀
     *
     * The bar is on the LEFT.
     * The triangle points LEFT.
     */

    SDL_FRect bar{
        centerX - 14.0f,
        centerY - BAR_HEIGHT / 2.0f,
        BAR_WIDTH,
        BAR_HEIGHT
    };

    SDL_RenderFillRect(
        sdlRenderer,
        &bar);

    SDL_Vertex vertices[3]{};

    vertices[0].position = {
        centerX + 10.0f,
        centerY - 11.0f
    };

    vertices[1].position = {
        centerX - 8.0f,
        centerY
    };

    vertices[2].position = {
        centerX + 10.0f,
        centerY + 11.0f
    };

    for (auto& vertex : vertices)
    {
        vertex.color = {
            0,
            0,
            0,
            255
        };
    }

    SDL_RenderGeometry(
        sdlRenderer,
        nullptr,
        vertices,
        3,
        nullptr,
        0);
}

// ============================================================
// Next button
// ============================================================

void MusicRenderer::renderNextButton(
    SDL_Renderer* sdlRenderer,
    float centerX,
    float centerY)
{
    if (sdlRenderer == nullptr)
        return;

    SDL_SetRenderDrawColor(
        sdlRenderer,
        0,
        0,
        0,
        255);

    constexpr float BAR_WIDTH = 4.0f;
    constexpr float BAR_HEIGHT = 22.0f;

    /*
     * Next = ▶|
     *
     * The triangle points RIGHT.
     * The bar is on the RIGHT.
     */

    SDL_Vertex vertices[3]{};

    vertices[0].position = {
        centerX - 10.0f,
        centerY - 11.0f
    };

    vertices[1].position = {
        centerX + 8.0f,
        centerY
    };

    vertices[2].position = {
        centerX - 10.0f,
        centerY + 11.0f
    };

    for (auto& vertex : vertices)
    {
        vertex.color = {
            0,
            0,
            0,
            255
        };
    }

    SDL_RenderGeometry(
        sdlRenderer,
        nullptr,
        vertices,
        3,
        nullptr,
        0);

    SDL_FRect bar{
        centerX + 10.0f,
        centerY - BAR_HEIGHT / 2.0f,
        BAR_WIDTH,
        BAR_HEIGHT
    };

    SDL_RenderFillRect(
        sdlRenderer,
        &bar);
}

// ============================================================
// Text rendering
// ============================================================

SDL_Texture* MusicRenderer::createTextTexture(
    SDL_Renderer* sdlRenderer,
    const std::string& text,
    TTF_Font* font,
    SDL_Color color)
{
    if (sdlRenderer == nullptr ||
        font == nullptr ||
        text.empty())
    {
        return nullptr;
    }

    SDL_Surface* surface =
        TTF_RenderText_Blended(
            font,
            text.c_str(),
            0,
            color);

    if (surface == nullptr)
    {
        SDL_Log(
            "MusicRenderer: text rendering failed: %s",
            SDL_GetError());

        return nullptr;
    }

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            sdlRenderer,
            surface);

    SDL_DestroySurface(surface);

    return texture;
}

void MusicRenderer::renderText(
    SDL_Renderer* sdlRenderer,
    const std::string& text,
    TTF_Font* font,
    SDL_Color color,
    float x,
    float y)
{
    SDL_Texture* texture =
        createTextTexture(
            sdlRenderer,
            text,
            font,
            color);

    if (texture == nullptr)
        return;

    float width = 0.0f;
    float height = 0.0f;

    SDL_GetTextureSize(
        texture,
        &width,
        &height);

    SDL_FRect destination{
        x,
        y,
        width,
        height
    };

    SDL_RenderTexture(
        sdlRenderer,
        texture,
        nullptr,
        &destination);

    SDL_DestroyTexture(texture);
}

void MusicRenderer::renderCenteredText(
    SDL_Renderer* sdlRenderer,
    const std::string& text,
    TTF_Font* font,
    SDL_Color color,
    float centerX,
    float y)
{
    if (font == nullptr)
        return;

    int width = 0;
    int height = 0;

    if (!TTF_GetStringSize(
            font,
            text.c_str(),
            0,
            &width,
            &height))
    {
        return;
    }

    const float x =
        centerX -
        static_cast<float>(width) / 2.0f;

    renderText(
        sdlRenderer,
        text,
        font,
        color,
        x,
        y);
}

// ============================================================
// Input
// ============================================================

bool MusicRenderer::pointInRect(
    float x,
    float y,
    const SDL_FRect& rect) const
{
    return
        x >= rect.x &&
        x <= rect.x + rect.w &&
        y >= rect.y &&
        y <= rect.y + rect.h;
}

void MusicRenderer::handleTouch(
    float x,
    float y,
    const SDL_FRect& bounds)
{
    if (!initialized)
        return;

    /*
     * Recreate the same vertical layout used by render().
     */

    float currentY =
        bounds.y +
        SPECTRUM_HEIGHT +
        SONG_INFO_HEIGHT;

    /*
     * --------------------------------------------------------
     * Progress bar
     * --------------------------------------------------------
     */

    SDL_FRect progressBounds{
        bounds.x,
        currentY,
        bounds.w,
        PROGRESS_HEIGHT
    };

    if (pointInRect(
            x,
            y,
            progressBounds))
    {
        seekFromPosition(
            x,
            progressBounds);

        return;
    }

    currentY +=
        PROGRESS_HEIGHT;

    /*
     * --------------------------------------------------------
     * Controls
     * --------------------------------------------------------
     */

    SDL_FRect controlsBounds{
        bounds.x,
        currentY,
        bounds.w,
        CONTROLS_HEIGHT
    };

    if (!pointInRect(
            x,
            y,
            controlsBounds))
    {
        return;
    }

    const float centerX =
        controlsBounds.x +
        controlsBounds.w / 2.0f;

    const float centerY =
        controlsBounds.y +
        controlsBounds.h / 2.0f;

    /*
     * Play / pause.
     */

    SDL_FRect playButton{
        centerX - 38.0f,
        centerY - 38.0f,
        76.0f,
        76.0f
    };

    if (pointInRect(
            x,
            y,
            playButton))
    {
        togglePlayPause();
        return;
    }

    /*
    * Previous.
    */

    SDL_FRect previousButton{
        centerX -
            CONTROL_SPACING -
            SIDE_BUTTON_SIZE / 2.0f,
        centerY -
            SIDE_BUTTON_SIZE / 2.0f,
        SIDE_BUTTON_SIZE,
        SIDE_BUTTON_SIZE
    };

    if (pointInRect(
            x,
            y,
            previousButton))
    {
        musicPlayer.previous();
        return;
    }

    /*
    * Next.
    */

    SDL_FRect nextButton{
        centerX +
            CONTROL_SPACING -
            SIDE_BUTTON_SIZE / 2.0f,
        centerY -
            SIDE_BUTTON_SIZE / 2.0f,
        SIDE_BUTTON_SIZE,
        SIDE_BUTTON_SIZE
    };

    if (pointInRect(
            x,
            y,
            nextButton))
    {
        musicPlayer.next();
        return;
    }
}

void MusicRenderer::handleMouseClick(
    float x,
    float y,
    const SDL_FRect& bounds)
{
    handleTouch(
        x,
        y,
        bounds);
}

// ============================================================
// Seek
// ============================================================

void MusicRenderer::seekFromPosition(
    float x,
    const SDL_FRect& progressBounds)
{
    const float duration =
        musicPlayer.getDuration();

    if (duration <= 0.0f)
        return;

    /*
     * Match the actual rendered progress track.
     */

    constexpr float TIME_WIDTH = 45.0f;

    const float barX =
        progressBounds.x +
        TIME_WIDTH;

    const float barWidth =
        progressBounds.w -
        TIME_WIDTH * 2.0f;

    float progress =
        (x - barX) /
        barWidth;

    progress =
        std::clamp(
            progress,
            0.0f,
            1.0f);

    musicPlayer.seek(
        progress * duration);
}

// ============================================================
// Playback
// ============================================================

void MusicRenderer::togglePlayPause()
{
    if (!initialized)
        return;

    musicPlayer.togglePlayPause();
}

// ============================================================
// Format time
// ============================================================

std::string MusicRenderer::formatTime(
    float seconds) const
{
    if (seconds < 0.0f)
        seconds = 0.0f;

    const int totalSeconds =
        static_cast<int>(seconds);

    const int minutes =
        totalSeconds / 60;

    const int remainingSeconds =
        totalSeconds % 60;

    std::ostringstream stream;

    stream
        << std::setfill('0')
        << std::setw(2)
        << minutes
        << ":"
        << std::setw(2)
        << remainingSeconds;

    return stream.str();
}