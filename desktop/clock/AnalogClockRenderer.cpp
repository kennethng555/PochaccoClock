#include "AnalogClockRenderer.hpp"

#include <cmath>
#include <algorithm>

namespace
{
    constexpr double PI =
        3.14159265358979323846;

    constexpr SDL_Color CLOCK_COLOR = {
        45, 45, 45, 255
    };

    constexpr SDL_Color MINUTE_COLOR = {
        110, 110, 110, 255
    };

    constexpr SDL_Color HOUR_COLOR = {
        35, 35, 35, 255
    };

    constexpr SDL_Color SECOND_COLOR = {
        190, 60, 60, 255
    };

    constexpr SDL_Color CENTER_COLOR = {
        35, 35, 35, 255
    };
}

AnalogClockRenderer::AnalogClockRenderer(
    SDL_Renderer* renderer)
    : renderer_(renderer)
{
}

void AnalogClockRenderer::render(
    const ClockTime& time,
    const SDL_FRect& bounds,
    float alpha)
{
    const float centerX =
        bounds.x + bounds.w / 2.0f;

    const float centerY =
        bounds.y + bounds.h / 2.0f;

    const float radius =
        std::min(bounds.w, bounds.h) / 2.0f - 5.0f;

    const Uint8 a =
        static_cast<Uint8>(
            std::clamp(alpha, 0.0f, 1.0f) * 255.0f
        );

    drawClockFace(
        centerX,
        centerY,
        radius,
        a
    );

    drawMinuteMarkers(
        centerX,
        centerY,
        radius,
        a
    );

    drawHourMarkers(
        centerX,
        centerY,
        radius,
        a
    );

    drawHands(
        time,
        centerX,
        centerY,
        radius,
        a
    );

    SDL_SetRenderDrawColor(
        renderer_,
        CENTER_COLOR.r,
        CENTER_COLOR.g,
        CENTER_COLOR.b,
        a
    );

    drawFilledCircle(
        centerX,
        centerY,
        5.0f,
        a
    );
}

void AnalogClockRenderer::drawClockFace(
    float centerX,
    float centerY,
    float radius,
    Uint8 alpha)
{
    SDL_SetRenderDrawColor(
        renderer_,
        CLOCK_COLOR.r,
        CLOCK_COLOR.g,
        CLOCK_COLOR.b,
        alpha
    );

    constexpr int segments = 180;

    for (int i = 0; i < segments; ++i) {

        const double a1 =
            2.0 * PI *
            static_cast<double>(i) /
            segments;

        const double a2 =
            2.0 * PI *
            static_cast<double>(i + 1) /
            segments;

        const int x1 =
            static_cast<int>(
                centerX +
                radius * std::sin(a1)
            );

        const int y1 =
            static_cast<int>(
                centerY +
                radius * std::cos(a1)
            );

        const int x2 =
            static_cast<int>(
                centerX +
                radius * std::sin(a2)
            );

        const int y2 =
            static_cast<int>(
                centerY +
                radius * std::cos(a2)
            );

        SDL_RenderLine(
            renderer_,
            x1,
            y1,
            x2,
            y2
        );
    }
}

void AnalogClockRenderer::drawMinuteMarkers(
    float centerX,
    float centerY,
    float radius,
    Uint8 alpha)
{
    SDL_SetRenderDrawColor(
        renderer_,
        MINUTE_COLOR.r,
        MINUTE_COLOR.g,
        MINUTE_COLOR.b,
        alpha
    );

    for (int minute = 0;
         minute < 60;
         ++minute) {

        if (minute % 5 == 0) {
            continue;
        }

        const double angle =
            2.0 * PI *
            static_cast<double>(minute) /
            60.0;

        constexpr float outerOffset = 8.0f;
        constexpr float innerOffset = 15.0f;

        const float outerRadius =
            radius - outerOffset;

        const float innerRadius =
            radius - innerOffset;

        const int x1 =
            static_cast<int>(
                centerX +
                innerRadius * std::sin(angle)
            );

        const int y1 =
            static_cast<int>(
                centerY -
                innerRadius * std::cos(angle)
            );

        const int x2 =
            static_cast<int>(
                centerX +
                outerRadius * std::sin(angle)
            );

        const int y2 =
            static_cast<int>(
                centerY -
                outerRadius * std::cos(angle)
            );

        SDL_RenderLine(
            renderer_,
            x1,
            y1,
            x2,
            y2
        );
    }
}

void AnalogClockRenderer::drawHourMarkers(
    float centerX,
    float centerY,
    float radius,
    Uint8 alpha)
{
    SDL_SetRenderDrawColor(
        renderer_,
        HOUR_COLOR.r,
        HOUR_COLOR.g,
        HOUR_COLOR.b,
        alpha
    );

    for (int hour = 0;
         hour < 12;
         ++hour) {

        const double angle =
            2.0 * PI *
            static_cast<double>(hour) /
            12.0;

        constexpr float outerOffset = 6.0f;
        constexpr float innerOffset = 22.0f;

        const float outerRadius =
            radius - outerOffset;

        const float innerRadius =
            radius - innerOffset;

        const int x1 =
            static_cast<int>(
                centerX +
                innerRadius * std::sin(angle)
            );

        const int y1 =
            static_cast<int>(
                centerY -
                innerRadius * std::cos(angle)
            );

        const int x2 =
            static_cast<int>(
                centerX +
                outerRadius * std::sin(angle)
            );

        const int y2 =
            static_cast<int>(
                centerY -
                outerRadius * std::cos(angle)
            );

        SDL_RenderLine(
            renderer_,
            x1,
            y1,
            x2,
            y2
        );
    }
}

void AnalogClockRenderer::drawHands(
    const ClockTime& time,
    float centerX,
    float centerY,
    float radius,
    Uint8 alpha)
{
    const double seconds =
        static_cast<double>(time.second) +
        time.fractionalSecond;

    const double minutes =
        static_cast<double>(time.minute) +
        seconds / 60.0;

    const double hours =
        static_cast<double>(time.hour % 12) +
        minutes / 60.0;

    const double hourAngle =
        2.0 * PI * hours / 12.0;

    const double minuteAngle =
        2.0 * PI * minutes / 60.0;

    const double secondAngle =
        2.0 * PI * seconds / 60.0;

    SDL_SetRenderDrawColor(
        renderer_,
        CLOCK_COLOR.r,
        CLOCK_COLOR.g,
        CLOCK_COLOR.b,
        alpha
    );

    drawHand(
        hourAngle,
        radius * 0.50f,
        5.0f,
        centerX,
        centerY,
        alpha
    );

    drawHand(
        minuteAngle,
        radius * 0.72f,
        4.0f,
        centerX,
        centerY,
        alpha
    );

    SDL_SetRenderDrawColor(
        renderer_,
        SECOND_COLOR.r,
        SECOND_COLOR.g,
        SECOND_COLOR.b,
        alpha
    );

    drawHand(
        secondAngle,
        radius * 0.82f,
        2.0f,
        centerX,
        centerY,
        alpha
    );
}

void AnalogClockRenderer::drawHand(
    double angle,
    float length,
    float width,
    float centerX,
    float centerY,
    Uint8 alpha)
{
    const float x =
        centerX +
        length *
        static_cast<float>(std::sin(angle));

    const float y =
        centerY -
        length *
        static_cast<float>(std::cos(angle));

    SDL_RenderLine(
        renderer_,
        static_cast<int>(centerX),
        static_cast<int>(centerY),
        static_cast<int>(x),
        static_cast<int>(y)
    );

    if (width >= 4.0f) {

        SDL_RenderLine(
            renderer_,
            static_cast<int>(centerX + 1),
            static_cast<int>(centerY),
            static_cast<int>(x + 1),
            static_cast<int>(y)
        );

        SDL_RenderLine(
            renderer_,
            static_cast<int>(centerX - 1),
            static_cast<int>(centerY),
            static_cast<int>(x - 1),
            static_cast<int>(y)
        );
    }
}

void AnalogClockRenderer::drawFilledCircle(
    float centerX,
    float centerY,
    float radius,
    Uint8 alpha)
{
    const int r =
        static_cast<int>(radius);

    const int cx =
        static_cast<int>(centerX);

    const int cy =
        static_cast<int>(centerY);

    for (int y = -r;
         y <= r;
         ++y) {

        const int width =
            static_cast<int>(
                std::sqrt(
                    radius * radius -
                    static_cast<float>(y * y)
                )
            );

        SDL_RenderLine(
            renderer_,
            cx - width,
            cy + y,
            cx + width,
            cy + y
        );
    }
}