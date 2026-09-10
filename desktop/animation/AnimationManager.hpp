#ifndef ANIMATION_MANAGER_HPP
#define ANIMATION_MANAGER_HPP

#include "AnimatedImage.hpp"

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

    ~AnimationManager() = default;

    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    bool initialize(SDL_Renderer* renderer);

    void update(float deltaTime);

    void render(SDL_Renderer* renderer);

    /*
     * Immediately show a specific animation.
     *
     * animationIndex must be 0-3.
     */
    void showAnimation(std::size_t animationIndex);

    /*
     * Hide the currently active animation.
     */
    void hideAnimation();

    /*
     * Enable/disable automatic random appearances.
     */
    void setRandomAppearancesEnabled(bool enabled);

    bool isActive() const;

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

    std::array<Animation, ANIMATION_COUNT> animations_;

    std::size_t currentAnimation_;

    bool active_;

    bool randomAppearancesEnabled_;

    /*
     * How long the currently selected animation
     * remains visible.
     */
    float displayTimer_;

    /*
     * Time remaining before another random animation
     * can appear.
     */
    float cooldownTimer_;

    /*
     * Time remaining until the next random appearance.
     */
    float nextAppearanceTimer_;

    /*
     * How long an animation stays visible.
     */
    static constexpr float DISPLAY_DURATION = 30.0f;

    /*
     * Minimum time between animations.
     */
    static constexpr float MIN_COOLDOWN = 60.0f;

    /*
     * Maximum time between animations.
     */
    static constexpr float MAX_COOLDOWN = 180.0f;

    std::mt19937 randomEngine_;

    float randomFloat(float minimum, float maximum);

    std::size_t randomAnimationIndex();

    void startRandomAnimation();

    void resetAppearanceTimer();
};

#endif