#pragma once

#include "AnimatedImage.hpp"
#include "../settings/Settings.hpp"
#include "../clock/ClockTime.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstddef>
#include <random>
#include <string>

class AnimationManager
{
public:
    static constexpr std::size_t ANIMATION_COUNT = 4;

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

    void showAnimation(
        const std::string& directory);

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

        SDL_FRect bounds{};

        std::string directory;

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