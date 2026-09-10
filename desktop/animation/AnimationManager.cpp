#include "AnimationManager.hpp"

#include <chrono>

AnimationManager::AnimationManager()
    : currentAnimation_(0),
      active_(false),
      randomAppearancesEnabled_(true),
      displayTimer_(0.0f),
      cooldownTimer_(0.0f),
      nextAppearanceTimer_(0.0f),
      randomEngine_(
          static_cast<std::mt19937::result_type>(
              std::chrono::steady_clock::now()
                  .time_since_epoch()
                  .count()))
{
    /*
     * Start with a random delay before the first
     * animation appears.
     */
    nextAppearanceTimer_ =
        randomFloat(
            MIN_COOLDOWN,
            MAX_COOLDOWN);
}

bool AnimationManager::initialize(
    SDL_Renderer* renderer)
{
    if (renderer == nullptr)
    {
        return false;
    }

    /*
     * --------------------------------------------------------
     * Animation 0 - Simba
     * --------------------------------------------------------
     *
     * Example:
     *
     * ../assets/Simba/frame_00.png
     * ../assets/Simba/frame_01.png
     * ...
     *
     * Change 16 to the actual number of frames.
     */
    if (animations_[0].image.load(
            renderer,
            "../assets/Simba",
            8))
    {
        animations_[0].loaded = true;
    }
    else
    {
        SDL_Log(
            "AnimationManager: failed to load Simba");
    }

    // /*
    //  * --------------------------------------------------------
    //  * Animation 1 - Pochacco
    //  * --------------------------------------------------------
    //  */
    // if (animations_[1].image.load(
    //         renderer,
    //         "../assets/Pochacco",
    //         16))
    // {
    //     animations_[1].loaded = true;
    // }
    // else
    // {
    //     SDL_Log(
    //         "AnimationManager: failed to load Pochacco");
    // }

    // /*
    //  * --------------------------------------------------------
    //  * Animation 2
    //  * --------------------------------------------------------
    //  */
    // if (animations_[2].image.load(
    //         renderer,
    //         "../assets/Animation3",
    //         16))
    // {
    //     animations_[2].loaded = true;
    // }
    // else
    // {
    //     SDL_Log(
    //         "AnimationManager: failed to load Animation3");
    // }

    // /*
    //  * --------------------------------------------------------
    //  * Animation 3
    //  * --------------------------------------------------------
    //  */
    // if (animations_[3].image.load(
    //         renderer,
    //         "../assets/Animation4",
    //         16))
    // {
    //     animations_[3].loaded = true;
    // }
    // else
    // {
    //     SDL_Log(
    //         "AnimationManager: failed to load Animation4");
    // }

    /*
     * Set the frame rate for each animation.
     *
     * 0.10f = 10 FPS.
     */
    for (Animation& animation : animations_)
    {
        if (animation.loaded)
        {
            /*
             * Your current AnimatedImage implementation
             * has frameDuration = 0.10f internally.
             *
             * No setter is required here.
             */
        }
    }

    /*
     * At least one animation must have loaded.
     */
    for (const Animation& animation : animations_)
    {
        if (animation.loaded)
        {
            return true;
        }
    }

    return false;
}

void AnimationManager::update(
    float deltaTime)
{
    if (deltaTime <= 0.0f)
    {
        return;
    }

    /*
     * --------------------------------------------------------
     * Currently active animation
     * --------------------------------------------------------
     */
    if (active_)
    {
        displayTimer_ += deltaTime;

        /*
         * Update the actual animation frames.
         */
        if (currentAnimation_ < ANIMATION_COUNT &&
            animations_[currentAnimation_].loaded)
        {
            animations_[currentAnimation_].image.update(
                deltaTime);
        }

        /*
         * Animation has been visible for 30 seconds.
         */
        if (displayTimer_ >= DISPLAY_DURATION)
        {
            hideAnimation();
        }

        return;
    }

    /*
     * --------------------------------------------------------
     * No animation currently visible.
     * --------------------------------------------------------
     */

    if (!randomAppearancesEnabled_)
    {
        return;
    }

    /*
     * Wait for the cooldown / random appearance delay.
     */
    if (nextAppearanceTimer_ > 0.0f)
    {
        nextAppearanceTimer_ -= deltaTime;

        if (nextAppearanceTimer_ > 0.0f)
        {
            return;
        }
    }

    /*
     * Time to show another animation.
     */
    startRandomAnimation();
}

void AnimationManager::render(
    SDL_Renderer* renderer)
{
    if (renderer == nullptr)
    {
        return;
    }

    if (!active_)
    {
        return;
    }

    if (currentAnimation_ >= ANIMATION_COUNT)
    {
        return;
    }

    Animation& animation =
        animations_[currentAnimation_];

    if (!animation.loaded)
    {
        return;
    }

    animation.image.render(
        renderer,
        animation.bounds);
}

void AnimationManager::showAnimation(
    std::size_t animationIndex)
{
    if (animationIndex >= ANIMATION_COUNT)
    {
        return;
    }

    if (!animations_[animationIndex].loaded)
    {
        SDL_Log(
            "AnimationManager: animation %zu is not loaded",
            animationIndex);

        return;
    }

    /*
     * Stop the currently active animation.
     */
    active_ = true;

    currentAnimation_ = animationIndex;

    /*
     * Start the animation from frame zero.
     */
    animations_[currentAnimation_].image.reset();

    /*
     * Reset the 30-second display timer.
     */
    displayTimer_ = 0.0f;

    /*
     * Don't immediately trigger another random
     * animation after this one finishes.
     */
    cooldownTimer_ = 0.0f;
}

void AnimationManager::hideAnimation()
{
    if (!active_)
    {
        return;
    }

    active_ = false;

    displayTimer_ = 0.0f;

    /*
     * Pick a new random delay before another
     * animation appears.
     */
    resetAppearanceTimer();
}

void AnimationManager::setRandomAppearancesEnabled(
    bool enabled)
{
    randomAppearancesEnabled_ = enabled;

    if (!enabled)
    {
        hideAnimation();
    }
}

bool AnimationManager::isActive() const
{
    return active_;
}

float AnimationManager::randomFloat(
    float minimum,
    float maximum)
{
    std::uniform_real_distribution<float> distribution(
        minimum,
        maximum);

    return distribution(randomEngine_);
}

std::size_t AnimationManager::randomAnimationIndex()
{
    /*
     * Build a list of loaded animations.
     *
     * This prevents an unloaded animation from ever
     * being selected.
     */
    std::array<std::size_t, ANIMATION_COUNT> loadedIndices{};

    std::size_t loadedCount = 0;

    for (std::size_t i = 0;
         i < ANIMATION_COUNT;
         ++i)
    {
        if (animations_[i].loaded)
        {
            loadedIndices[loadedCount] = i;
            ++loadedCount;
        }
    }

    if (loadedCount == 0)
    {
        return 0;
    }

    std::uniform_int_distribution<std::size_t> distribution(
        0,
        loadedCount - 1);

    return loadedIndices[distribution(randomEngine_)];
}

void AnimationManager::startRandomAnimation()
{
    const std::size_t animationIndex =
        randomAnimationIndex();

    /*
     * Make sure the selected animation is actually loaded.
     */
    if (!animations_[animationIndex].loaded)
    {
        return;
    }

    showAnimation(animationIndex);
}

void AnimationManager::resetAppearanceTimer()
{
    nextAppearanceTimer_ =
        randomFloat(
            MIN_COOLDOWN,
            MAX_COOLDOWN);
}