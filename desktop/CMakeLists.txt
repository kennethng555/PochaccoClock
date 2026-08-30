cmake_minimum_required(VERSION 3.20)

project(PochaccoClockDesktop
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------
# SDL3
# --------------------------------------------------

set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(
    SDL
    EXCLUDE_FROM_ALL
)

# --------------------------------------------------
# SDL3_image
# --------------------------------------------------

set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(
    SDL_image
    EXCLUDE_FROM_ALL
)

# --------------------------------------------------
# SDL3_ttf
# --------------------------------------------------

set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDLTTF_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(
    SDL_ttf
    EXCLUDE_FROM_ALL
)

# --------------------------------------------------
# Application
# --------------------------------------------------

add_executable(PochaccoClock

    src/main.cpp
    src/AnalogClockRenderer.cpp
    src/DigitalClockRenderer.cpp
    src/ClockManager.cpp
    src/SDL3Display.cpp

    ../shared/Clock.cpp
)

target_include_directories(PochaccoClock
    PRIVATE
        ../shared
)

target_link_libraries(PochaccoClock
    PRIVATE
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
)