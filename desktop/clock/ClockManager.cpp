#include "ClockManager.hpp"

ClockManager::ClockManager(
    SDL_Renderer* renderer,
    const std::string& fontPath)
    : renderer_(renderer),
      digitalRenderer_(
          std::make_unique<DigitalClockRenderer>(
              renderer,
              fontPath)),
      elapsedTime_(0.0f)
{
}

ClockManager::~ClockManager() = default;

bool ClockManager::initialize(
    const ClockSettings& settings)
{
    if (!digitalRenderer_)
    {
        return false;
    }

    if (!digitalRenderer_->initialize())
    {
        return false;
    }

    /*
     * Animations are decorative, so failure to load
     * them should not prevent the clock from starting.
     */
    if (!animationManager_.initialize(
            renderer_,
            settings))
    {
        SDL_Log(
            "ClockManager: no animations loaded");
    }

    return true;
}

void ClockManager::update(
    float deltaTime,
    const ClockSettings& settings,
    const ClockTime& time)
{
    elapsedTime_ += deltaTime;

    animationManager_.update(
        deltaTime,
        settings,
        time);
}

void ClockManager::render(
    const ClockTime& time,
    const SDL_FRect& bounds,
    TimeOfDay timeOfDay)
{
    if (!digitalRenderer_)
    {
        return;
    }

    digitalRenderer_->render(
        time,
        bounds,
        timeOfDay);

    animationManager_.render(
        renderer_);
}

AnimationManager& ClockManager::getAnimationManager()
{
    return animationManager_;
}