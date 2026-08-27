#include <SDL3/SDL.h>

#include <iostream>

#include "Clock.hpp"

constexpr int SCREEN_WIDTH = 600;
constexpr int SCREEN_HEIGHT = 450;

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr
            << "SDL_Init failed: "
            << SDL_GetError()
            << '\n';

        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!SDL_CreateWindowAndRenderer(
            "Pochacco Clock",
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            0,
            &window,
            &renderer)) {

        std::cerr
            << "SDL_CreateWindowAndRenderer failed: "
            << SDL_GetError()
            << '\n';

        SDL_Quit();
        return 1;
    }

    Clock clock;

    bool running = true;

    while (running) {

        SDL_Event event;

        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {

                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        clock.update();

        const ClockTime& time = clock.time();

        // Temporary output so we know the
        // shared clock is working.
        static int lastSecond = -1;

        if (time.second != lastSecond) {

            std::cout
                << time.hour
                << ':'
                << time.minute
                << ':'
                << time.second
                << '\n';

            lastSecond = time.second;
        }

        // Background
        SDL_SetRenderDrawColor(
            renderer,
            240,
            240,
            240,
            255
        );

        SDL_RenderClear(renderer);

        // Clock rendering will go here.

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}