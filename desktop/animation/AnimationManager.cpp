#include "AnimationManager.hpp"

#include <chrono>

AnimationManager::AnimationManager()
    : randomEngine_(
          static_cast<std::mt19937::result_type>(
              std::chrono::steady_clock::now()
                  .time_since_epoch()
                  .count()))
{
    resetAppearanceTimer();
}

bool AnimationManager::initialize(
    SDL_Renderer* renderer,
    const ClockSettings& settings)
{
    if (renderer == nullptr)
    {
        return false;
    }

    bool anyLoaded = false;

    for (std::size_t i = 0; i < ANIMATION_COUNT; ++i) {
        const AnimationConfig& config = settings.animations[i];

        if (!config.enabled) {
            continue;
        }

        if (config.frameCount == 0) {
            continue;
        }

        if (animations_[i].image.load(renderer, config.directory, config.frameCount)) {
            animations_[i].loaded = true;
            animations_[i].bounds = {
                config.x,
                config.y,
                config.width,
                config.height
            };

            anyLoaded = true;
        } else {
            SDL_Log("AnimationManager: failed to load %s", config.name.c_str());
        }
    }

    return anyLoaded;
}

void AnimationManager::update(
    float deltaTime,
    const ClockSettings& settings,
    const ClockTime& time)
{
    if (deltaTime <= 0.0f)
    {
        return;
    }

    /*
     * Currently displaying an animation.
     */
    if (active_)
    {
        displayTimer_ += deltaTime;

        if (currentAnimation_ < ANIMATION_COUNT)
        {
            Animation& animation =
                animations_[currentAnimation_];

            if (animation.loaded)
            {
                animation.image.update(
                    deltaTime);
            }

            const float duration =
                settings
                    .animations[currentAnimation_]
                    .displayDuration;

            if (displayTimer_ >= duration)
            {
                hideAnimation();
            }
        }

        return;
    }

    /*
     * Scheduled animations have priority over
     * random animations.
     */
    if (checkScheduledAnimations(
            settings,
            time))
    {
        return;
    }

    /*
     * No scheduled animation was triggered,
     * so random animation behavior can proceed.
     */
    nextAppearanceTimer_ -= deltaTime;

    if (nextAppearanceTimer_ > 0.0f)
    {
        return;
    }

    startRandomAnimation(settings);
}

void AnimationManager::render(
    SDL_Renderer* renderer)
{
    if (renderer == nullptr || !active_) {
        return;
    }

    if (currentAnimation_ >= ANIMATION_COUNT) {
        return;
    }

    Animation& animation = animations_[currentAnimation_];

    if (!animation.loaded) {
        return;
    }

    animation.image.render(renderer, animation.bounds);
}

void AnimationManager::showAnimation(
    std::size_t animationIndex)
{
    if (animationIndex >= ANIMATION_COUNT) {
        return;
    }

    if (!animations_[animationIndex].loaded) {
        return;
    }

    currentAnimation_ = animationIndex;

    animations_[currentAnimation_].image.reset();

    displayTimer_ = 0.0f;

    active_ = true;
}

void AnimationManager::hideAnimation()
{
    active_ = false;

    displayTimer_ = 0.0f;

    resetAppearanceTimer();
}

bool AnimationManager::isActive() const
{
    return active_;
}

std::size_t AnimationManager::getCurrentAnimation() const
{
    return currentAnimation_;
}

float AnimationManager::randomFloat(
    float minimum,
    float maximum)
{
    std::uniform_real_distribution<float>
        distribution(
            minimum,
            maximum);

    return distribution(randomEngine_);
}

std::size_t AnimationManager::randomAnimationIndex(
    const ClockSettings& settings)
{
    std::array<std::size_t, ANIMATION_COUNT>
        available{};

    std::size_t count = 0;

    for (std::size_t i = 0; i < ANIMATION_COUNT; ++i) {
        const AnimationConfig& config = settings.animations[i];

        if (!config.enabled || !config.randomEnabled || !animations_[i].loaded) {
            continue;
        }

        available[count] = i;
        ++count;
    }

    if (count == 0)
    {
        return ANIMATION_COUNT;
    }

    std::uniform_int_distribution<std::size_t> distribution(0, count - 1);

    return available[distribution(randomEngine_)];
}

void AnimationManager::startRandomAnimation(
    const ClockSettings& settings)
{
    const std::size_t index =
        randomAnimationIndex(settings);

    if (index >= ANIMATION_COUNT) {
        resetAppearanceTimer();
        return;
    }

    showAnimation(index);
}

void AnimationManager::resetAppearanceTimer()
{
    /*
     * Random appearance every 1–3 minutes.
     */
    nextAppearanceTimer_ = randomFloat(60.0f, 180.0f);
}

bool AnimationManager::checkScheduledAnimations(
    const ClockSettings& settings,
    const ClockTime& time)
{
    /*
     * Only process each clock minute once.
     */
    if (time.hour == lastCheckedHour_ &&
        time.minute == lastCheckedMinute_)
    {
        return false;
    }

    lastCheckedHour_ = time.hour;
    lastCheckedMinute_ = time.minute;

    for (std::size_t i = 0;
         i < ANIMATION_COUNT;
         ++i)
    {
        const AnimationConfig& config =
            settings.animations[i];

        if (!config.enabled ||
            !config.scheduled ||
            !animations_[i].loaded)
        {
            continue;
        }

        if (config.scheduledHour == time.hour &&
            config.scheduledMinute == time.minute)
        {
            showAnimation(i);
            return true;
        }
    }

    return false;
}