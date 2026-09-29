#include "ClockManager.hpp"

#include <SDL3/SDL.h>

namespace
{
constexpr float BIRTHDAY_CENTER_X =
    300.0f;

constexpr float BIRTHDAY_TITLE_Y =
    18.0f;

constexpr float BIRTHDAY_NAME_Y =
    58.0f;
}

ClockManager::ClockManager(
    SDL_Renderer* renderer,
    const std::string& fontPath)
    : renderer_(renderer),
      digitalRenderer_(
          std::make_unique<
              DigitalClockRenderer>(
              renderer,
              fontPath)),
      elapsedTime_(0.0f),
      fontPath_(fontPath)
{
}

ClockManager::~ClockManager()
{
    if (birthdayNameFont_ != nullptr)
    {
        TTF_CloseFont(
            birthdayNameFont_);

        birthdayNameFont_ = nullptr;
    }

    if (birthdayFont_ != nullptr)
    {
        TTF_CloseFont(
            birthdayFont_);

        birthdayFont_ = nullptr;
    }
}

bool ClockManager::initialize(
    const ClockSettings& settings)
{
    if (!digitalRenderer_)
        return false;

    if (!digitalRenderer_->initialize())
        return false;

    /*
     * Birthday text uses the same font as the clock.
     *
     * Failure to load these fonts does not prevent the
     * clock itself from initializing.
     */
    birthdayFont_ =
        TTF_OpenFont(
            fontPath_.c_str(),
            50.0f);

    birthdayNameFont_ =
        TTF_OpenFont(
            fontPath_.c_str(),
            45.0f);

    if (birthdayFont_ == nullptr)
    {
        SDL_Log(
            "ClockManager: failed to load birthday font: %s",
            SDL_GetError());
    }

    if (birthdayNameFont_ == nullptr)
    {
        SDL_Log(
            "ClockManager: failed to load birthday name font: %s",
            SDL_GetError());
    }

    if (!animationManager_.initialize(
            renderer_,
            settings))
    {
        return false;
    }

    return true;
}

void ClockManager::update(
    float deltaTime,
    const ClockSettings& settings,
    const ClockTime& time)
{
    elapsedTime_ += deltaTime;

    /*
     * Birthday animation is updated separately.
     *
     * The four normal animations should not run during
     * birthday mode.
     */
    if (birthdayActive_)
        return;

    animationManager_.update(
        deltaTime,
        settings,
        time);
}

void ClockManager::updateBirthday(
    float deltaTime)
{
    if (!birthdayActive_)
        return;

    animationManager_.updateBirthday(
        deltaTime);
}

void ClockManager::render(
    const ClockTime& time,
    const SDL_FRect& bounds,
    TimeOfDay timeOfDay)
{
    if (!digitalRenderer_)
        return;

    /*
     * Birthday text is drawn first.
     */
    if (birthdayActive_)
    {
        renderBirthdayText();
    }

    /*
     * The normal digital clock is ALWAYS rendered.
     */
    digitalRenderer_->render(
        time,
        bounds,
        timeOfDay);

    /*
     * Normal decorative animations are suppressed
     * during birthday mode.
     */
    if (!birthdayActive_)
    {
        animationManager_.render(
            renderer_);
    }
}

AnimationManager&
ClockManager::getAnimationManager()
{
    return animationManager_;
}

// ============================================================
// Birthday
// ============================================================

void ClockManager::startBirthday(const BirthdaySettings& settings)
{
    if (!settings.enabled)
        return;

    birthdaySettings_ = settings;
    birthdayActive_ = true;

    digitalRenderer_->setBirthdayMode(true);

    animationManager_.startBirthday(settings);

    SDL_Log(
        "ClockManager: birthday mode started for %s",
        birthdaySettings_.name.c_str()
    );
}

void ClockManager::stopBirthday()
{
    if (!birthdayActive_)
        return;

    birthdayActive_ = false;

    digitalRenderer_->setBirthdayMode(false);

    animationManager_.stopBirthday();

    SDL_Log("ClockManager: birthday mode stopped");
}

bool ClockManager::isBirthdayActive() const
{
    return birthdayActive_;
}

void ClockManager::renderBirthdayBackground()
{
    if (!birthdayActive_)
        return;

    animationManager_.renderBirthday(
        renderer_);
}

// ============================================================
// Birthday text
// ============================================================

void ClockManager::renderBirthdayText()
{
    if (!renderer_ || !birthdayActive_)
        return;

    const std::string title = "HAPPY BIRTHDAY!";
    const std::string name = birthdaySettings_.name;

    const SDL_Color textColor{255, 255, 255, 255};
    const SDL_Color outlineColor{113, 74, 45, 255};

    auto renderCenteredText =
        [&](TTF_Font* font,
            const std::string& text,
            float centerX,
            float y)
    {
        if (!font || text.empty())
            return;

        /*
         * Render the outline version.
         */
        SDL_Surface* outlineSurface =
            TTF_RenderText_Blended(
                font,
                text.c_str(),
                0,
                outlineColor);

        /*
         * Render the actual white letters.
         */
        SDL_Surface* textSurface =
            TTF_RenderText_Blended(
                font,
                text.c_str(),
                0,
                textColor);

        if (!outlineSurface || !textSurface)
        {
            if (outlineSurface)
                SDL_DestroySurface(outlineSurface);

            if (textSurface)
                SDL_DestroySurface(textSurface);

            return;
        }

        SDL_Texture* outlineTexture =
            SDL_CreateTextureFromSurface(
                renderer_,
                outlineSurface);

        SDL_Texture* textTexture =
            SDL_CreateTextureFromSurface(
                renderer_,
                textSurface);

        if (outlineTexture && textTexture)
        {
            const float textX =
                centerX -
                static_cast<float>(textSurface->w) / 2.0f;

            const float textY = y;

            /*
             * Draw the black outline in all directions.
             *
             * Because the copies are only offset by 2 pixels,
             * they merge into a border around the letters.
             */
            constexpr float outline = 3.0f;

            const float outlineX =
                centerX -
                static_cast<float>(outlineSurface->w) / 2.0f;

            SDL_FRect outlineBounds{
                outlineX,
                textY,
                static_cast<float>(outlineSurface->w),
                static_cast<float>(outlineSurface->h)
            };

            const float offsets[][2] = {
                {-outline, -outline},
                { 0.0f,   -outline},
                { outline, -outline},
                {-outline,  0.0f},
                { outline,  0.0f},
                {-outline,  outline},
                { 0.0f,    outline},
                { outline,  outline}
            };

            for (const auto& offset : offsets)
            {
                outlineBounds.x =
                    outlineX + offset[0];

                outlineBounds.y =
                    textY + offset[1];

                SDL_RenderTexture(
                    renderer_,
                    outlineTexture,
                    nullptr,
                    &outlineBounds);
            }

            /*
             * Draw the actual white letters on top.
             */
            SDL_FRect textBounds{
                textX,
                textY,
                static_cast<float>(textSurface->w),
                static_cast<float>(textSurface->h)
            };

            SDL_RenderTexture(
                renderer_,
                textTexture,
                nullptr,
                &textBounds);
        }

        if (outlineTexture)
            SDL_DestroyTexture(outlineTexture);

        if (textTexture)
            SDL_DestroyTexture(textTexture);

        SDL_DestroySurface(outlineSurface);
        SDL_DestroySurface(textSurface);
    };

    constexpr float centerX = 300.0f;

    renderCenteredText(
        birthdayFont_,
        title,
        centerX,
        325.0f);

    renderCenteredText(
        birthdayNameFont_,
        name,
        centerX,
        375.0f);
}