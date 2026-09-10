#include "ClockManager.hpp"

ClockManager::ClockManager(
    SDL_Renderer* renderer,
    const std::string& fontPath)
    : renderer_(renderer),
      digitalRenderer_(
          std::make_unique<DigitalClockRenderer>(
              renderer,
              fontPath
          )
      ),
      elapsedTime_(0.0f)
{
}

ClockManager::~ClockManager() = default;

bool ClockManager::initialize()
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
     * Initialize the decorative animations.
     *
     * Failure to load animations does not prevent
     * the clock itself from working.
     */
    if (!animationManager_.initialize(renderer_))
    {
        SDL_Log(
            "ClockManager: no animations could be loaded");
    }

    return true;
}

void ClockManager::update(
    float deltaTime)
{
    elapsedTime_ += deltaTime;

    /*
     * Update the animation scheduler and
     * currently active animation.
     */
    animationManager_.update(deltaTime);
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

    /*
     * Render the normal clock.
     */
    digitalRenderer_->render(
        time,
        bounds,
        timeOfDay
    );

    /*
     * Render the currently active decorative
     * animation, if there is one.
     */
    animationManager_.render(
        renderer_);
}