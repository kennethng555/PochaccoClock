#pragma once

#include <SDL3/SDL.h>

namespace AppLayout
{

// ============================================================
// Logical resolution
// ============================================================

constexpr int LOGICAL_WIDTH  = 600;
constexpr int LOGICAL_HEIGHT = 450;

// ============================================================
// Pochacco scaling
// ============================================================

constexpr float SCALE_FACTOR = 0.4f;

constexpr float POCHACCO_WIDTH =
    1370.0f * SCALE_FACTOR;

constexpr float POCHACCO_HEIGHT =
    548.0f * SCALE_FACTOR;

// ============================================================
// Permanent scene layout
// ============================================================

// Music spectrum analyzer: top-left.
constexpr SDL_FRect MUSIC_BOUNDS = {
    20.0f,
    50.0f,
    560.0f,
    350.0f
};

// Digital clock: center-left.
constexpr SDL_FRect CLOCK_BOUNDS = {
    LOGICAL_WIDTH * 0.1f,
    LOGICAL_HEIGHT * 0.25f,
    270.0f,
    230.0f
};

// ============================================================
// Music Box button
// ============================================================

constexpr SDL_FRect MUSIC_BOX_BUTTON_BOUNDS = {
    LOGICAL_WIDTH * 0.5f - 145.0f / 2.0f,
    LOGICAL_HEIGHT * 0.1f - 48.0f / 2.0f,
    145.0f,
    48.0f
};

constexpr SDL_Color MUSIC_BOX_BUTTON_COLOR = {
    184,
    220,
    180,
    255
};

constexpr SDL_Color MUSIC_BOX_BUTTON_ACTIVE_COLOR = {
    145,
    195,
    150,
    255
};

constexpr SDL_Color MUSIC_BOX_TEXT_COLOR = {
    50,
    80,
    55,
    255
};

// ============================================================
// Pochacco + bed
// ============================================================

constexpr SDL_FRect POCHACCO_BOUNDS = {
    LOGICAL_WIDTH - POCHACCO_WIDTH + 65.0f,
    LOGICAL_HEIGHT - POCHACCO_HEIGHT,
    POCHACCO_WIDTH,
    POCHACCO_HEIGHT
};

// ============================================================
// Birthday Configuration
// ============================================================

constexpr SDL_FRect BIRTHDAY_MUSIC_BOX_BUTTON_BOUNDS = {
    227.5f,
    45.0f,
    145.0f,
    48.0f
};

constexpr SDL_FRect BIRTHDAY_CLOCK_BOUNDS = {
    60.0f,
    112.5f,
    270.0f,
    230.0f
};

// ============================================================
// Assets
// ============================================================

constexpr const char* POCHACCO_PATH =
    "../assets/pochacco-green.png";

constexpr const char* MORNING_PATH =
    "../assets/morning.jpg";

constexpr const char* DAYTIME_PATH =
    "../assets/daytime.jpg";

constexpr const char* EVENING_PATH =
    "../assets/evening.jpg";

constexpr const char* NIGHT_PATH =
    "../assets/night.jpg";

constexpr const char* FONT_PATH =
    "../assets/fonts/PinkyBlues.otf";

inline SDL_FRect musicBoxButtonBounds(bool birthday)
{
    return birthday
        ? BIRTHDAY_MUSIC_BOX_BUTTON_BOUNDS
        : MUSIC_BOX_BUTTON_BOUNDS;
}

inline SDL_FRect clockBounds(bool birthday)
{
    return birthday
        ? BIRTHDAY_CLOCK_BOUNDS
        : CLOCK_BOUNDS;
}

inline SDL_FRect fullScreenBounds()
{
    return {
        0.0f,
        0.0f,
        static_cast<float>(LOGICAL_WIDTH),
        static_cast<float>(LOGICAL_HEIGHT)
    };
}

}