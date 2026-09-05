#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <cstddef>

class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer() = default;
    ~SpectrumAnalyzer() = default;

    void update(
        const float* samples,
        size_t count);

    void render(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds);

private:
    static constexpr size_t FFT_SIZE = 512;
    static constexpr size_t NUM_BARS = 16;

    // Raw PCM samples from the audio source.
    std::array<float, FFT_SIZE> sampleBuffer{};

    // Calculated frequency spectrum.
    //
    // Only half of the FFT/DFT output is useful for
    // real-valued audio signals.
    std::array<float, FFT_SIZE / 2> spectrum{};

    // Current displayed magnitudes.
    std::array<float, NUM_BARS> magnitudes{};

    // Target magnitudes calculated from the spectrum.
    std::array<float, NUM_BARS> targetMagnitudes{};

    float magnitudeForBin(size_t bin) const;

    void calculateSpectrum();
    void calculateBands();
    void smoothBands();
};