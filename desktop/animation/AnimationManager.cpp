#include "AnimationManager.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>

namespace
{
constexpr const char* ANIMATIONS_DIRECTORY =
    "../assets/animations";
}

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

    namespace fs = std::filesystem;

    animations_.clear();

    currentAnimation_ = 0;
    debugAnimationIndex_ = 0;

    active_ = false;
    displayTimer_ = 0.0f;

    const fs::path animationRoot =
        ANIMATIONS_DIRECTORY;

    if (!fs::exists(animationRoot) ||
        !fs::is_directory(animationRoot))
    {
        SDL_Log(
            "AnimationManager: animation directory not found: %s",
            animationRoot.string().c_str());

        return false;
    }

    std::vector<fs::path> directories;

    for (const auto& entry :
         fs::directory_iterator(animationRoot))
    {
        if (!entry.is_directory())
            continue;

        directories.push_back(
            entry.path());
    }

    /*
     * directory_iterator does not guarantee ordering.
     *
     * Sort the directories so the debug animation order
     * remains deterministic.
     */
    std::sort(
        directories.begin(),
        directories.end());

    SDL_Log(
        "AnimationManager: found %zu animation directories",
        directories.size());

    for (const fs::path& directoryPath :
         directories)
    {
        Animation animation;

        animation.loaded = false;

        animation.directory =
            directoryPath.string();

        // ----------------------------------------------------
        // Find matching settings entry.
        // ----------------------------------------------------

        const AnimationConfig* config =
            nullptr;

        for (const AnimationConfig& candidate :
             settings.animations)
        {
            if (candidate.directory ==
                animation.directory)
            {
                config = &candidate;
                break;
            }
        }

        if (config == nullptr)
        {
            SDL_Log(
                "AnimationManager: no settings entry for %s",
                animation.directory.c_str());

            continue;
        }

        if (!config->enabled)
        {
            SDL_Log(
                "AnimationManager: animation disabled: %s",
                animation.directory.c_str());

            continue;
        }

        if (config->frameCount == 0)
        {
            SDL_Log(
                "AnimationManager: animation has no frames: %s",
                animation.directory.c_str());

            continue;
        }

        animation.bounds = SDL_FRect{
            config->x,
            config->y,
            config->width,
            config->height
        };

        // ----------------------------------------------------
        // Load animation.
        // ----------------------------------------------------

        if (animation.image.load(
                renderer,
                animation.directory,
                config->frameCount))
        {
            animation.loaded = true;

            SDL_Log(
                "AnimationManager: loaded animation %zu: %s",
                animations_.size(),
                animation.directory.c_str());

            animations_.push_back(
                std::move(animation));
        }
        else
        {
            SDL_Log(
                "AnimationManager: failed to load animation: %s",
                animation.directory.c_str());
        }
    }

    // ========================================================
    // Birthday animation
    // ========================================================

    birthdayLoaded_ = false;
    birthdayActive_ = false;

    /*
     * Birthday animation is deliberately NOT part of the
     * normal animation directory scan.
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
        if (currentAnimation_ < animations_.size())
        {
            Animation& animation =
                animations_[currentAnimation_];

            if (animation.loaded)
            {
                animation.image.update(
                    deltaTime);
            }

            displayTimer_ += deltaTime;

            /*
             * Settings are matched by directory rather than
             * assuming the settings index matches the animation
             * vector index.
             */
            const AnimationConfig* config =
                nullptr;

            for (const AnimationConfig& candidate :
                 settings.animations)
            {
                if (candidate.directory ==
                    animation.directory)
                {
                    config = &candidate;
                    break;
                }
            }

            if (config != nullptr)
            {
                const float displayDuration =
                    config->displayDuration;

                if (displayDuration > 0.0f &&
                    displayTimer_ >= displayDuration)
                {
                    hideAnimation();
                }
            }
        }
    }
    else
    {
        nextAppearanceTimer_ -=
            deltaTime;

        if (nextAppearanceTimer_ <= 0.0f)
        {
            startRandomAnimation(
                settings);

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

    if (currentAnimation_ >= animations_.size())
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
    if (animationIndex >= animations_.size())
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
         i < animations_.size();
         ++i)
    {
        if (animations_[i].directory ==
            directory)
        {
            showAnimation(i);
            return;
        }
    }

    SDL_Log(
        "AnimationManager: animation not found: %s",
        directory.c_str());
}

// ============================================================
// Debug animation
// ============================================================

void AnimationManager::showNextAnimation()
{
    if (animations_.empty())
    {
        SDL_Log(
            "DEBUG: No animations loaded");

        return;
    }

    if (debugAnimationIndex_ >=
        animations_.size())
    {
        debugAnimationIndex_ = 0;
    }

    const std::size_t index =
        debugAnimationIndex_;

    SDL_Log(
        "DEBUG: Showing animation %zu/%zu: %s",
        index + 1,
        animations_.size(),
        animations_[index].directory.c_str());

    /*
     * Use showAnimation() rather than modifying
     * currentAnimation_ directly.
     *
     * This ensures the selected animation is the one
     * that AnimationManager::render() will render.
     */
    showAnimation(index);

    debugAnimationIndex_ =
        (debugAnimationIndex_ + 1) %
        animations_.size();
}

// ============================================================
// Hide normal animation
// ============================================================

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
    std::uniform_real_distribution<float>
        distribution(
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
         i < animations_.size();
         ++i)
    {
        const Animation& animation =
            animations_[i];

        if (!animation.loaded)
            continue;

        const AnimationConfig* config =
            nullptr;

        for (const AnimationConfig& candidate :
             settings.animations)
        {
            if (candidate.directory ==
                animation.directory)
            {
                config = &candidate;
                break;
            }
        }

        if (config == nullptr)
            continue;

        if (!config->randomEnabled)
            continue;

        candidates.push_back(i);
    }

    if (candidates.empty())
        return animations_.size();

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

    if (index >= animations_.size())
        return;

    showAnimation(index);
}

void AnimationManager::resetAppearanceTimer()
{
    nextAppearanceTimer_ =
        randomFloat(
            60.0f,
            180.0f);
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
         i < animations_.size();
         ++i)
    {
        Animation& animation =
            animations_[i];

        const AnimationConfig* config =
            nullptr;

        for (const AnimationConfig& candidate :
             settings.animations)
        {
            if (candidate.directory ==
                animation.directory)
            {
                config = &candidate;
                break;
            }
        }

        if (config == nullptr)
            continue;

        if (!config->scheduled)
            continue;

        if (config->scheduledHour != time.hour ||
            config->scheduledMinute != time.minute)
        {
            continue;
        }

        if (!animation.loaded)
            continue;

        showAnimation(i);

        return true;
    }

    return false;
}