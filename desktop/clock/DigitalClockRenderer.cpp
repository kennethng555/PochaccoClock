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

    constexpr float AMPM_SPACING = 8.0f;
    constexpr float TIME_Y_OFFSET = 25.0f;
    constexpr float DATE_Y_OFFSET = 105.0f;

    constexpr float DIGIT_SLOT_WIDTH = 43.0f;
    constexpr float COLON_SLOT_WIDTH = 24.0f;

    constexpr int TEXT_OUTLINE_WIDTH = 3;
    constexpr SDL_Color NORMAL_OUTLINE_COLOR = {0, 0, 0, 255};
    constexpr SDL_Color BIRTHDAY_OUTLINE_COLOR = {255, 255, 255, 255};

    // --------------------------------------------------------
    // Birthday colors
    // --------------------------------------------------------

    constexpr SDL_Color BIRTHDAY_TIME_COLOR = {113, 74, 45, 255};
    constexpr SDL_Color BIRTHDAY_SMALL_COLOR = {113, 74, 45, 255};
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

    SDL_Color timeColor;
    SDL_Color smallColor;
    SDL_Color outlineColor;

    if (birthdayMode_) {

        timeColor = BIRTHDAY_TIME_COLOR;
        smallColor = BIRTHDAY_SMALL_COLOR;
        outlineColor = BIRTHDAY_OUTLINE_COLOR;

    } else {

        switch (timeOfDay) {

            case TimeOfDay::Morning:
            case TimeOfDay::Day:
            case TimeOfDay::Evening:
                timeColor = {0, 0, 0, 255};
                smallColor = {0, 0, 0, 255};
                outlineColor = {255, 255, 255, 255};
                break;

            case TimeOfDay::Night:
                timeColor = {255, 255, 255, 255};
                smallColor = {255, 255, 255, 255};
                outlineColor = {0, 0, 0, 255};
                break;
        }
    }

    drawTime(
        time,
        bounds,
        timeColor,
        outlineColor);

    drawAmPm(
        time,
        bounds,
        smallColor,
        outlineColor);

    drawDate(
        time,
        bounds,
        smallColor,
        outlineColor);
}

// ============================================================
// Draw time
// ============================================================

void DigitalClockRenderer::drawTime(
    const ClockTime& time,
    const SDL_FRect& bounds,
    SDL_Color textColor,
    SDL_Color outlineColor)
{
    int hour = time.hour % 12;

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

    constexpr int DIGIT_COUNT = 6;
    constexpr int COLON_COUNT = 2;

    const float timeWidth =
        DIGIT_COUNT * DIGIT_SLOT_WIDTH +
        COLON_COUNT * COLON_SLOT_WIDTH;

    const float startX =
        bounds.x +
        (bounds.w - timeWidth) / 2.0f;

    const float y =
        bounds.y +
        TIME_Y_OFFSET;

    float currentX = startX;

    for (int i = 0; buffer[i] != '\0'; ++i) {

        const char character = buffer[i];

        const float slotWidth =
            character == ':'
                ? COLON_SLOT_WIDTH
                : DIGIT_SLOT_WIDTH;

        char characterBuffer[2]{
            character,
            '\0'
        };

        // ----------------------------------------------------
        // Render the outlined version.
        //
        // The outline surface is larger than the normal
        // glyph. We position it TEXT_OUTLINE_WIDTH pixels
        // above/left so its inner glyph lines up with the
        // normal glyph.
        // ----------------------------------------------------

        TTF_SetFontOutline(
            timeFont_,
            TEXT_OUTLINE_WIDTH
        );

        SDL_Surface* outlineSurface =
            TTF_RenderText_Blended(
                timeFont_,
                characterBuffer,
                0,
                outlineColor
            );

        TTF_SetFontOutline(
            timeFont_,
            0
        );

        // ----------------------------------------------------
        // Render the normal glyph.
        // ----------------------------------------------------

        SDL_Surface* textSurface =
            TTF_RenderText_Blended(
                timeFont_,
                characterBuffer,
                0,
                textColor
            );

        if (!outlineSurface || !textSurface) {

            std::cerr
                << "TTF_RenderText_Blended failed: "
                << SDL_GetError()
                << '\n';

            if (outlineSurface)
                SDL_DestroySurface(outlineSurface);

            if (textSurface)
                SDL_DestroySurface(textSurface);

            currentX += slotWidth;
            continue;
        }

        SDL_Texture* outlineTexture =
            SDL_CreateTextureFromSurface(
                renderer_,
                outlineSurface
            );

        SDL_Texture* textTexture =
            SDL_CreateTextureFromSurface(
                renderer_,
                textSurface
            );

        if (!outlineTexture || !textTexture) {

            std::cerr
                << "SDL_CreateTextureFromSurface failed: "
                << SDL_GetError()
                << '\n';

            if (outlineTexture)
                SDL_DestroyTexture(outlineTexture);

            if (textTexture)
                SDL_DestroyTexture(textTexture);

            SDL_DestroySurface(outlineSurface);
            SDL_DestroySurface(textSurface);

            currentX += slotWidth;
            continue;
        }

        // ----------------------------------------------------
        // Position based on the NORMAL glyph.
        //
        // This preserves your existing fixed-width layout.
        // ----------------------------------------------------

        const float glyphWidth =
            static_cast<float>(textSurface->w);

        const float glyphHeight =
            static_cast<float>(textSurface->h);

        const float glyphX =
            currentX +
            (slotWidth - glyphWidth) / 2.0f;

        // ----------------------------------------------------
        // Draw black outline.
        // ----------------------------------------------------

        SDL_FRect outlineDestination{};

        outlineDestination.x =
            glyphX -
            TEXT_OUTLINE_WIDTH;

        outlineDestination.y =
            y -
            TEXT_OUTLINE_WIDTH;

        outlineDestination.w =
            static_cast<float>(outlineSurface->w);

        outlineDestination.h =
            static_cast<float>(outlineSurface->h);

        SDL_RenderTexture(
            renderer_,
            outlineTexture,
            nullptr,
            &outlineDestination
        );

        // ----------------------------------------------------
        // Draw the normal glyph directly over the center.
        // ----------------------------------------------------

        SDL_FRect textDestination{};

        textDestination.x = glyphX;
        textDestination.y = y;
        textDestination.w = glyphWidth;
        textDestination.h = glyphHeight;

        SDL_RenderTexture(
            renderer_,
            textTexture,
            nullptr,
            &textDestination
        );

        SDL_DestroyTexture(outlineTexture);
        SDL_DestroyTexture(textTexture);

        SDL_DestroySurface(outlineSurface);
        SDL_DestroySurface(textSurface);

        currentX += slotWidth;
    }
}

// ============================================================
// Draw AM / PM
// ============================================================

void DigitalClockRenderer::drawAmPm(
    const ClockTime& time,
    const SDL_FRect& bounds,
    SDL_Color textColor,
    SDL_Color outlineColor)
{
    const char* ampm =
        time.hour >= 12
            ? "PM"
            : "AM";

    // --------------------------------------------------------
    // Render AM / PM surface.
    // --------------------------------------------------------

    TTF_SetFontOutline(
        smallFont_,
        TEXT_OUTLINE_WIDTH
    );

    SDL_Surface* outlineSurface =
        TTF_RenderText_Blended(
            smallFont_,
            ampm,
            0,
            outlineColor
        );

    TTF_SetFontOutline(
        smallFont_,
        0
    );

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

    SDL_Texture* outlineTexture =
        SDL_CreateTextureFromSurface(
            renderer_,
            outlineSurface
        );

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

    SDL_FRect outlineDestination = destination;

    outlineDestination.x -= TEXT_OUTLINE_WIDTH;
    outlineDestination.y -= TEXT_OUTLINE_WIDTH;

    outlineDestination.w =
        static_cast<float>(outlineSurface->w);

    outlineDestination.h =
        static_cast<float>(outlineSurface->h);

    SDL_RenderTexture(
        renderer_,
        outlineTexture,
        nullptr,
        &outlineDestination
    );

    SDL_RenderTexture(
        renderer_,
        texture,
        nullptr,
        &destination
    );

    SDL_DestroyTexture(outlineTexture);
    SDL_DestroyTexture(texture);

    SDL_DestroySurface(outlineSurface);
    SDL_DestroySurface(surface);
}

// ============================================================
// Draw date
// ============================================================

void DigitalClockRenderer::drawDate(
    const ClockTime& time,
    const SDL_FRect& bounds,
    SDL_Color textColor,
    SDL_Color outlineColor)
{
    std::tm date{};

    date.tm_year = time.year - 1900;
    date.tm_mon  = time.month - 1;
    date.tm_mday = time.day;

    // Calculate weekday and normalize the tm structure.
    std::mktime(&date);

    char buffer[64];

    std::strftime(
        buffer,
        sizeof(buffer),
        "%a %b %d",
        &date
    );

    // --------------------------------------------------------
    // Calculate the fixed-width time area.
    // --------------------------------------------------------

    constexpr int DIGIT_COUNT = 6;
    constexpr int COLON_COUNT = 2;

    const float timeWidth =
        DIGIT_COUNT * DIGIT_SLOT_WIDTH +
        COLON_COUNT * COLON_SLOT_WIDTH;

    const float timeStartX =
        bounds.x +
        (bounds.w - timeWidth) / 2.0f;

    const float x = timeStartX;
    const float y = bounds.y + DATE_Y_OFFSET;

    // --------------------------------------------------------
    // Render black outline.
    // --------------------------------------------------------

    TTF_SetFontOutline(
        smallFont_,
        TEXT_OUTLINE_WIDTH
    );

    SDL_Surface* outlineSurface =
        TTF_RenderText_Blended(
            smallFont_,
            buffer,
            0,
            outlineColor
        );

    // Immediately reset the font.
    TTF_SetFontOutline(
        smallFont_,
        0
    );

    // --------------------------------------------------------
    // Render normal date text.
    // --------------------------------------------------------

    SDL_Surface* textSurface =
        TTF_RenderText_Blended(
            smallFont_,
            buffer,
            0,
            textColor
        );

    if (!outlineSurface || !textSurface) {

        std::cerr
            << "Failed to render date text: "
            << SDL_GetError()
            << '\n';

        if (outlineSurface)
            SDL_DestroySurface(outlineSurface);

        if (textSurface)
            SDL_DestroySurface(textSurface);

        return;
    }

    SDL_Texture* outlineTexture =
        SDL_CreateTextureFromSurface(
            renderer_,
            outlineSurface
        );

    SDL_Texture* textTexture =
        SDL_CreateTextureFromSurface(
            renderer_,
            textSurface
        );

    if (!outlineTexture || !textTexture) {

        std::cerr
            << "Failed to create date textures: "
            << SDL_GetError()
            << '\n';

        if (outlineTexture)
            SDL_DestroyTexture(outlineTexture);

        if (textTexture)
            SDL_DestroyTexture(textTexture);

        SDL_DestroySurface(outlineSurface);
        SDL_DestroySurface(textSurface);

        return;
    }

    // --------------------------------------------------------
    // Normal text position.
    // --------------------------------------------------------

    const float textWidth =
        static_cast<float>(textSurface->w);

    const float textHeight =
        static_cast<float>(textSurface->h);

    SDL_FRect textDestination{};

    textDestination.x = x;
    textDestination.y = y;
    textDestination.w = textWidth;
    textDestination.h = textHeight;

    // --------------------------------------------------------
    // Outline position.
    //
    // The outlined surface is larger by 2 * outline width.
    // Offset it so the inner glyph lines up exactly with the
    // normal text.
    // --------------------------------------------------------

    SDL_FRect outlineDestination{};

    outlineDestination.x =
        x - TEXT_OUTLINE_WIDTH;

    outlineDestination.y =
        y - TEXT_OUTLINE_WIDTH;

    outlineDestination.w =
        static_cast<float>(outlineSurface->w);

    outlineDestination.h =
        static_cast<float>(outlineSurface->h);

    // --------------------------------------------------------
    // Draw outline first.
    // --------------------------------------------------------

    SDL_RenderTexture(
        renderer_,
        outlineTexture,
        nullptr,
        &outlineDestination
    );

    // --------------------------------------------------------
    // Draw date over the center of the outline.
    // --------------------------------------------------------

    SDL_RenderTexture(
        renderer_,
        textTexture,
        nullptr,
        &textDestination
    );

    SDL_DestroyTexture(outlineTexture);
    SDL_DestroyTexture(textTexture);

    SDL_DestroySurface(outlineSurface);
    SDL_DestroySurface(textSurface);
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

void DigitalClockRenderer::setBirthdayMode(bool enabled)
{
    birthdayMode_ = enabled;
}