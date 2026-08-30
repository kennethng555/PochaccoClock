#include "SDL3Display.hpp"

#include <iostream>

SDL3Display::SDL3Display()
    : window_(nullptr),
      renderer_(nullptr)
{
}

SDL3Display::~SDL3Display()
{
    shutdown();
}

bool SDL3Display::initialize(
    const char* title,
    int width,
    int height)
{
    // --------------------------------------------------------
    // SDL
    // --------------------------------------------------------

    if (!SDL_Init(SDL_INIT_VIDEO)) {

        std::cerr
            << "SDL_Init failed: "
            << SDL_GetError()
            << '\n';

        return false;
    }

    // --------------------------------------------------------
    // Window
    // --------------------------------------------------------

    window_ =
        SDL_CreateWindow(
            title,
            width,
            height,
            SDL_WINDOW_RESIZABLE
        );

    if (!window_) {

        std::cerr
            << "SDL_CreateWindow failed: "
            << SDL_GetError()
            << '\n';

        SDL_Quit();

        return false;
    }

    // --------------------------------------------------------
    // Renderer
    // --------------------------------------------------------

    renderer_ =
        SDL_CreateRenderer(
            window_,
            nullptr
        );

    if (!renderer_) {

        std::cerr
            << "SDL_CreateRenderer failed: "
            << SDL_GetError()
            << '\n';

        SDL_DestroyWindow(window_);
        window_ = nullptr;

        SDL_Quit();

        return false;
    }

    return true;
}

void SDL3Display::beginFrame(
    SDL_Color background)
{
    SDL_SetRenderDrawColor(
        renderer_,
        background.r,
        background.g,
        background.b,
        background.a
    );

    SDL_RenderClear(renderer_);
}

void SDL3Display::endFrame()
{
    SDL_RenderPresent(renderer_);
}

bool SDL3Display::processEvents(
    bool& running)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        switch (event.type) {

            case SDL_EVENT_QUIT:

                running = false;
                break;

            default:

                break;
        }
    }

    return running;
}

SDL_Renderer*
SDL3Display::renderer()
{
    return renderer_;
}

SDL_Window*
SDL3Display::window()
{
    return window_;
}

void SDL3Display::shutdown()
{
    if (renderer_) {

        SDL_DestroyRenderer(
            renderer_
        );

        renderer_ = nullptr;
    }

    if (window_) {

        SDL_DestroyWindow(
            window_
        );

        window_ = nullptr;
    }

    SDL_Quit();
}