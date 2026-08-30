#include "DigitalClockRenderer.hpp"

#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>

// ============================================================
// Layout constants
// ============================================================

namespace
{
    constexpr int TIME_FONT_SIZE = 72;
    constexpr int SMALL_FONT_SIZE = 36;

    // Space between the time and AM/PM.
    constexpr float AMPM_SPACING = 8.0f;

    // Vertical position of the time within clockBounds.
    constexpr float TIME_Y_OFFSET = 25.0f;

    // Vertical position of the date.
    constexpr float DATE_Y_OFFSET = 105.0f;

    // Space allocated to each digit.
    //
    // The actual glyph can be narrower than this, but the
    // position of every digit remains fixed.
    constexpr float DIGIT_SLOT_WIDTH = 43.0f;

    // Space around ':'.
    constexpr float COLON_SLOT_WIDTH = 24.0f;
}

// ============================================================
// Constructor
// ============================================================

DigitalClockRenderer::DigitalClockRenderer(
    SDL_Renderer* renderer,
    const std::string& fontPath)
    : renderer_(renderer),
      fontPath_(fontPath),
      timeFont_(nullptr),
      smallFont_(nullptr)
{
}

// ============================================================
// Destructor
// ============================================================

DigitalClockRenderer::~DigitalClockRenderer()
{
    if (timeFont_) {
        TTF_CloseFont(timeFont_);
        timeFont_ = nullptr;
    }

    if (smallFont_) {
        TTF_CloseFont(smallFont_);
        smallFont_ = nullptr;
    }
}

// ============================================================
// Initialize
// ============================================================

bool DigitalClockRenderer::initialize()
{
    // --------------------------------------------------------
    // Main time font
    // --------------------------------------------------------

    timeFont_ =
        TTF_OpenFont(
            fontPath_.c_str(),
            TIME_FONT_SIZE
        );

    if (!timeFont_) {

        std::cerr
            << "Failed to load time font: "
            << SDL_GetError()
            << '\n';

        return false;
    }

    // --------------------------------------------------------
    // Smaller font
    //
    // Used for:
    //   AM / PM
    //   Day + date
    //
    // 36px = half of the 72px time font.
    // --------------------------------------------------------

    smallFont_ =
        TTF_OpenFont(
            fontPath_.c_str(),
            SMALL_FONT_SIZE
        );

    if (!smallFont_) {

        std::cerr
            << "Failed to load small font: "
            << SDL_GetError()
            << '\n';

        TTF_CloseFont(timeFont_);
        timeFont_ = nullptr;

        return false;
    }

    return true;
}

// ============================================================
// Render
// ============================================================

void DigitalClockRenderer::render(
    const ClockTime& time,
    const SDL_FRect& bounds,
    TimeOfDay timeOfDay)
{
    if (!timeFont_ || !smallFont_) {
        return;
    }

    SDL_Color textColor;

    switch (timeOfDay) {

        case TimeOfDay::Morning:
        case TimeOfDay::Day:
        case TimeOfDay::Evening:
            textColor = SDL_Color{0, 0, 0, 255};
            break;

        case TimeOfDay::Night:
            textColor = SDL_Color{255, 255, 255, 255};
            break;
    }

    drawTime(time, bounds, textColor);
    drawAmPm(time, bounds, textColor);
    drawDate(time, bounds, textColor);
}

// ============================================================
// Draw time
// ============================================================

void DigitalClockRenderer::drawTime(
    const ClockTime& time,
    const SDL_FRect& bounds,
    SDL_Color textColor)
{
    // --------------------------------------------------------
    // Convert to 12-hour format.
    // --------------------------------------------------------

    int hour =
        time.hour % 12;

    if (hour == 0) {
        hour = 12;
    }

    char buffer[32];

    std::snprintf(
        buffer,
        sizeof(buffer),
        "%02d:%02d:%02d",
        hour,
        time.minute,
        time.second
    );

    // --------------------------------------------------------
    // Calculate total width.
    //
    // Format:
    //
    //     0 3 : 4 5 : 2 7
    //
    // Every digit receives the same slot width.
    // --------------------------------------------------------

    constexpr int DIGIT_COUNT = 6;
    constexpr int COLON_COUNT = 2;

    const float timeWidth =
        DIGIT_COUNT * DIGIT_SLOT_WIDTH +
        COLON_COUNT * COLON_SLOT_WIDTH;

    // --------------------------------------------------------
    // Center the fixed-width time area.
    // --------------------------------------------------------

    const float startX =
        bounds.x +
        (
            bounds.w -
            timeWidth
        ) / 2.0f;

    const float y =
        bounds.y +
        TIME_Y_OFFSET;

    float currentX = startX;

    // --------------------------------------------------------
    // Render each character.
    // --------------------------------------------------------

    for (int i = 0; buffer[i] != '\0'; ++i) {

        const char character =
            buffer[i];

        // ----------------------------------------------------
        // Determine slot width.
        // ----------------------------------------------------

        const float slotWidth =
            character == ':'
                ? COLON_SLOT_WIDTH
                : DIGIT_SLOT_WIDTH;

        // ----------------------------------------------------
        // Render character.
        // ----------------------------------------------------

        char characterBuffer[2]{
            character,
            '\0'
        };

        SDL_Surface* surface =
            TTF_RenderText_Blended(
                timeFont_,
                characterBuffer,
                0,
                textColor
            );

        if (!surface) {

            std::cerr
                << "TTF_RenderText_Blended failed: "
                << SDL_GetError()
                << '\n';

            currentX += slotWidth;

            continue;
        }

        SDL_Texture* texture =
            SDL_CreateTextureFromSurface(
                renderer_,
                surface
            );

        if (!texture) {

            std::cerr
                << "SDL_CreateTextureFromSurface failed: "
                << SDL_GetError()
                << '\n';

            SDL_DestroySurface(surface);

            currentX += slotWidth;

            continue;
        }

        // ----------------------------------------------------
        // Center the glyph inside its fixed slot.
        // ----------------------------------------------------

        const float glyphWidth =
            static_cast<float>(
                surface->w
            );

        const float glyphHeight =
            static_cast<float>(
                surface->h
            );

        SDL_FRect destination{};

        destination.w =
            glyphWidth;

        destination.h =
            glyphHeight;

        destination.x =
            currentX +
            (
                slotWidth -
                glyphWidth
            ) / 2.0f;

        destination.y =
            y;

        SDL_RenderTexture(
            renderer_,
            texture,
            nullptr,
            &destination
        );

        SDL_DestroyTexture(texture);
        SDL_DestroySurface(surface);

        // ----------------------------------------------------
        // Advance by the fixed slot width.
        // ----------------------------------------------------

        currentX += slotWidth;
    }
}

// ============================================================
// Draw AM / PM
// ============================================================

void DigitalClockRenderer::drawAmPm(
    const ClockTime& time,
    const SDL_FRect& bounds,
    SDL_Color textColor)
{
    const char* ampm =
        time.hour >= 12
            ? "PM"
            : "AM";

    // --------------------------------------------------------
    // Render AM / PM surface.
    // --------------------------------------------------------

    SDL_Surface* surface =
        TTF_RenderText_Blended(
            smallFont_,
            ampm,
            0,
            textColor
        );

    if (!surface) {

        std::cerr
            << "TTF_RenderText_Blended failed: "
            << SDL_GetError()
            << '\n';

        return;
    }

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            renderer_,
            surface
        );

    if (!texture) {

        std::cerr
            << "SDL_CreateTextureFromSurface failed: "
            << SDL_GetError()
            << '\n';

        SDL_DestroySurface(surface);

        return;
    }

    // --------------------------------------------------------
    // Calculate the fixed width of the time.
    // --------------------------------------------------------

    constexpr int DIGIT_COUNT = 6;
    constexpr int COLON_COUNT = 2;

    const float timeWidth =
        DIGIT_COUNT * DIGIT_SLOT_WIDTH +
        COLON_COUNT * COLON_SLOT_WIDTH;

    // --------------------------------------------------------
    // Calculate the complete width:
    //
    //     03:45:21 [space] PM
    // --------------------------------------------------------

    const float totalWidth =
        timeWidth +
        AMPM_SPACING +
        static_cast<float>(surface->w);

    const float startX =
        bounds.x +
        (
            bounds.w -
            totalWidth
        ) / 2.0f;

    // --------------------------------------------------------
    // AM/PM position.
    // --------------------------------------------------------

    const float timeY =
        bounds.y +
        TIME_Y_OFFSET;

    const float centerX =
        bounds.x +
        bounds.w / 2.0f;

    const float timeHeight =
        static_cast<float>(
            TTF_GetFontHeight(timeFont_)
        );

    const float timeX =
        centerX -
        static_cast<float>(timeWidth) / 2.0f;

    constexpr float spacing = 8.0f;

    SDL_FRect destination{};

    destination.w =
        static_cast<float>(surface->w);

    destination.h =
        static_cast<float>(surface->h);

    destination.x =
        timeX +
        static_cast<float>(timeWidth) +
        spacing;

    // Vertically center AM/PM against the main time.
    destination.y =
        bounds.y +
        10 +
        (
            static_cast<float>(timeHeight) -
            destination.h
        );

    SDL_RenderTexture(
        renderer_,
        texture,
        nullptr,
        &destination
    );

    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

// ============================================================
// Draw date
// ============================================================

void DigitalClockRenderer::drawDate(
    const ClockTime& time,
    const SDL_FRect& bounds,
    SDL_Color textColor)
{
    std::tm date{};

    date.tm_year = time.year - 1900;
    date.tm_mon  = time.month - 1;
    date.tm_mday = time.day;

    char buffer[64];

    std::strftime(
        buffer,
        sizeof(buffer),
        "%A, %B %d",
        &date
    );

    // --------------------------------------------------------
    // Calculate the fixed-width time area.
    //
    // The date will start at the exact same X position
    // as the first digit of the time.
    // --------------------------------------------------------

    constexpr int DIGIT_COUNT = 6;
    constexpr int COLON_COUNT = 2;

    const float timeWidth =
        DIGIT_COUNT * DIGIT_SLOT_WIDTH +
        COLON_COUNT * COLON_SLOT_WIDTH;

    const float timeStartX =
        bounds.x +
        (
            bounds.w -
            timeWidth
        ) / 2.0f;

    // --------------------------------------------------------
    // Date
    // --------------------------------------------------------

    drawText(
        smallFont_,
        buffer,
        textColor,
        timeStartX,
        bounds.y + DATE_Y_OFFSET,
        false
    );
}


// ============================================================
// Draw centered text
// ============================================================

void DigitalClockRenderer::drawText(
    TTF_Font* font,
    const char* text,
    SDL_Color color,
    float x,
    float y,
    bool centered)
{
    if (!font || !text) {
        return;
    }

    SDL_Surface* surface =
        TTF_RenderText_Blended(
            font,
            text,
            0,
            color
        );

    if (!surface) {

        std::cerr
            << "TTF_RenderText_Blended failed: "
            << SDL_GetError()
            << '\n';

        return;
    }

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            renderer_,
            surface
        );

    if (!texture) {

        std::cerr
            << "SDL_CreateTextureFromSurface failed: "
            << SDL_GetError()
            << '\n';

        SDL_DestroySurface(surface);

        return;
    }

    SDL_FRect destination{};

    destination.w =
        static_cast<float>(surface->w);

    destination.h =
        static_cast<float>(surface->h);

    // --------------------------------------------------------
    // Horizontal alignment
    // --------------------------------------------------------

    if (centered) {

        destination.x =
            x -
            destination.w / 2.0f;

    } else {

        destination.x =
            x;
    }

    destination.y = y;

    SDL_RenderTexture(
        renderer_,
        texture,
        nullptr,
        &destination
    );

    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}