cmake_minimum_required(VERSION 3.20)

project(PochaccoClockDesktop
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(
    SDL
    EXCLUDE_FROM_ALL
)

add_executable(PochaccoClock
    src/main.cpp
    ../shared/Clock.cpp
)

target_include_directories(PochaccoClock
    PRIVATE
        ../shared
)

target_link_libraries(PochaccoClock
    PRIVATE
        SDL3::SDL3
)