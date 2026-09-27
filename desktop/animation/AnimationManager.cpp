#include "AnimationManager.hpp"

#include <algorithm>
#include <chrono>

AnimationManager::AnimationManager()
{
    std::random_device rd;

    randomEngine_.seed(rd());

    resetAppearanceTimer();
}

bool AnimationManager::initialize(
    SDL_Renderer* renderer,
    const ClockSettings& settings)
{
    if (renderer == nullptr)
        return false;

    // ========================================================
    // Normal animations
    // ========================================================

    for (std::size_t i = 0;
         i < ANIMATION_COUNT;
         ++i)
    {
        Animation& animation =
            animations_[i];

        animation.loaded = false;
        animation.directory.clear();

        const AnimationConfig& config =
            settings.animations[i];

        animation.bounds = SDL_FRect{
            config.x,
            config.y,
            config.width,
            config.height
        };

        animation.directory =
            config.directory;

        if (!config.enabled)
            continue;

        if (config.directory.empty())
            continue;

        if (config.frameCount == 0)
            continue;

        if (animation.image.load(
                renderer,
                config.directory,
                config.frameCount))
        {
            animation.loaded = true;

            SDL_Log(
                "AnimationManager: loaded animation %zu: %s",
                i,
                config.name.c_str());
        }
        else
        {
            SDL_Log(
                "AnimationManager: failed to load animation %zu: %s",
                i,
                config.name.c_str());
        }
    }

    // ========================================================
    // Birthday animation
    // ========================================================

    birthdayLoaded_ = false;
    birthdayActive_ = false;

    /*
     * Birthday is deliberately NOT part of the four normal
     * animation slots.
     */
    if (!settings.birthday.animationDirectory.empty() &&
        settings.birthday.frameCount > 0)
    {
        if (birthdayImage_.load(
                renderer,
                settings.birthday.animationDirectory,
                settings.birthday.frameCount))
        {
            birthdayLoaded_ = true;

            SDL_Log(
                "AnimationManager: loaded birthday animation: %s",
                settings.birthday.animationDirectory.c_str());
        }
        else
        {
            SDL_Log(
                "AnimationManager: failed to load birthday animation");
        }
    }

    return true;
}

// ============================================================
// Normal animation update
// ============================================================

void AnimationManager::update(
    float deltaTime,
    const ClockSettings& settings,
    const ClockTime& time)
{
    if (active_)
    {
        if (currentAnimation_ < ANIMATION_COUNT)
        {
            Animation& animation =
                animations_[currentAnimation_];

            if (animation.loaded)
            {
                animation.image.update(deltaTime);
            }

            displayTimer_ += deltaTime;

            const float displayDuration =
                settings.animations[
                    currentAnimation_
                ].displayDuration;

            if (displayDuration > 0.0f &&
                displayTimer_ >= displayDuration)
            {
                hideAnimation();
            }
        }
    }
    else
    {
        nextAppearanceTimer_ -= deltaTime;

        if (nextAppearanceTimer_ <= 0.0f)
        {
            startRandomAnimation(settings);
            resetAppearanceTimer();
        }
    }

    checkScheduledAnimations(
        settings,
        time);
}

// ============================================================
// Normal animation render
// ============================================================

void AnimationManager::render(
    SDL_Renderer* renderer)
{
    if (!active_)
        return;

    if (currentAnimation_ >= ANIMATION_COUNT)
        return;

    Animation& animation =
        animations_[currentAnimation_];

    if (!animation.loaded)
        return;

    animation.image.render(
        renderer,
        animation.bounds);
}

// ============================================================
// Show normal animation
// ============================================================

void AnimationManager::showAnimation(
    std::size_t animationIndex)
{
    if (animationIndex >= ANIMATION_COUNT)
        return;

    Animation& animation =
        animations_[animationIndex];

    if (!animation.loaded)
        return;

    currentAnimation_ =
        animationIndex;

    animation.image.reset();

    displayTimer_ = 0.0f;

    active_ = true;
}

void AnimationManager::showAnimation(
    const std::string& directory)
{
    for (std::size_t i = 0;
         i < ANIMATION_COUNT;
         ++i)
    {
        if (animations_[i].directory == directory)
        {
            showAnimation(i);
            return;
        }
    }
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

// ============================================================
// Birthday animation
// ============================================================

void AnimationManager::startBirthday(
    const BirthdaySettings& settings)
{
    if (!settings.enabled)
        return;

    /*
     * Birthday mode takes over from the normal decorative
     * animation system.
     */
    hideAnimation();

    birthdayImage_.reset();

    birthdayActive_ = true;

    SDL_Log(
        "AnimationManager: birthday mode started");
}

void AnimationManager::stopBirthday()
{
    birthdayActive_ = false;

    birthdayImage_.reset();

    SDL_Log(
        "AnimationManager: birthday mode stopped");
}

void AnimationManager::updateBirthday(
    float deltaTime)
{
    if (!birthdayActive_)
        return;

    if (!birthdayLoaded_)
        return;

    birthdayImage_.update(
        deltaTime);
}

void AnimationManager::renderBirthday(
    SDL_Renderer* renderer)
{
    if (!birthdayActive_)
        return;

    if (!birthdayLoaded_)
        return;

    birthdayImage_.render(
        renderer,
        birthdayBounds_);
}

bool AnimationManager::isBirthdayActive() const
{
    return birthdayActive_;
}

// ============================================================
// Random animation helpers
// ============================================================

float AnimationManager::randomFloat(
    float minimum,
    float maximum)
{
    std::uniform_real_distribution<float> distribution(
        minimum,
        maximum);

    return distribution(
        randomEngine_);
}

std::size_t AnimationManager::randomAnimationIndex(
    const ClockSettings& settings)
{
    std::vector<std::size_t> candidates;

    for (std::size_t i = 0;
         i < ANIMATION_COUNT;
         ++i)
    {
        const AnimationConfig& config =
            settings.animations[i];

        if (!config.enabled)
            continue;

        if (!animations_[i].loaded)
            continue;

        if (!config.randomEnabled)
            continue;

        candidates.push_back(i);
    }

    if (candidates.empty())
        return ANIMATION_COUNT;

    std::uniform_int_distribution<std::size_t>
        distribution(
            0,
            candidates.size() - 1);

    return candidates[
        distribution(randomEngine_)
    ];
}

void AnimationManager::startRandomAnimation(
    const ClockSettings& settings)
{
    const std::size_t index =
        randomAnimationIndex(settings);

    if (index >= ANIMATION_COUNT)
        return;

    showAnimation(index);
}

void AnimationManager::resetAppearanceTimer()
{
    nextAppearanceTimer_ =
        randomFloat(60.0f, 180.0f);
}

// ============================================================
// Scheduled animations
// ============================================================

bool AnimationManager::checkScheduledAnimations(
    const ClockSettings& settings,
    const ClockTime& time)
{
    if (time.hour == lastCheckedHour_ &&
        time.minute == lastCheckedMinute_)
    {
        return false;
    }

    lastCheckedHour_ =
        time.hour;

    lastCheckedMinute_ =
        time.minute;

    for (std::size_t i = 0;
         i < ANIMATION_COUNT;
         ++i)
    {
        const AnimationConfig& config =
            settings.animations[i];

        if (!config.enabled)
            continue;

        if (!config.scheduled)
            continue;

        if (config.scheduledHour != time.hour ||
            config.scheduledMinute != time.minute)
        {
            continue;
        }

        if (!animations_[i].loaded)
            continue;

        showAnimation(i);

        return true;
    }

    return false;
}