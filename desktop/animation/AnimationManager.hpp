#pragma once

#include "AnimatedImage.hpp"
#include "../settings/Settings.hpp"
#include "../clock/ClockTime.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstddef>
#include <random>

class AnimationManager
{
public:
    static constexpr std::size_t ANIMATION_COUNT = 1;

    AnimationManager();

    bool initialize(
        SDL_Renderer* renderer,
        const ClockSettings& settings);

    void update(
        float deltaTime,
        const ClockSettings& settings,
        const ClockTime& time);

    void render(
        SDL_Renderer* renderer);

    void showAnimation(
        std::size_t animationIndex);

    void hideAnimation();

    bool isActive() const;

    std::size_t getCurrentAnimation() const;

    bool checkScheduledAnimations(
        const ClockSettings& settings,
        const ClockTime& time);

private:
    struct Animation
    {
        AnimatedImage image;

        SDL_FRect bounds{
            5.0f,
            325.0f,
            150.0f,
            112.5f
        };

        bool loaded = false;
    };

    std::array<Animation, ANIMATION_COUNT>
        animations_;

    std::size_t currentAnimation_ = 0;

    bool active_ = false;

    float displayTimer_ = 0.0f;

    float nextAppearanceTimer_ = 30.0f;

    std::mt19937 randomEngine_;

    float randomFloat(
        float minimum,
        float maximum);

    std::size_t randomAnimationIndex(
        const ClockSettings& settings);

    void startRandomAnimation(
        const ClockSettings& settings);

    void resetAppearanceTimer();

    int lastCheckedHour_ = -1;
    int lastCheckedMinute_ = -1;
};