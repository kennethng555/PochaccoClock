#include "ClockManager.hpp"

ClockManager::ClockManager(
    SDL_Renderer* renderer,
    const std::string& fontPath)
    : digitalRenderer_(
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
    if (!digitalRenderer_) {
        return false;
    }

    return digitalRenderer_->initialize();
}

void ClockManager::update(
    float deltaTime)
{
    elapsedTime_ += deltaTime;
}

void ClockManager::render(
    const ClockTime& time,
    const SDL_FRect& bounds,
    TimeOfDay timeOfDay)
{
    if (!digitalRenderer_) {
        return;
    }

    digitalRenderer_->render(
        time,
        bounds,
        timeOfDay
    );
}