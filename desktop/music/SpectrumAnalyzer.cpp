#include "SpectrumAnalyzer.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float PI =
        3.14159265358979323846f;

    constexpr float ATTACK = 0.35f;
    constexpr float DECAY  = 0.08f;
}

void SpectrumAnalyzer::update(
    const float* samples,
    size_t count)
{
    if (samples == nullptr)
        return;

    // Copy incoming PCM samples.
    const size_t samplesToCopy =
        std::min(count, FFT_SIZE);

    std::copy_n(
        samples,
        samplesToCopy,
        sampleBuffer.begin());

    // Clear unused portion.
    if (samplesToCopy < FFT_SIZE)
    {
        std::fill(
            sampleBuffer.begin() + samplesToCopy,
            sampleBuffer.end(),
            0.0f);
    }

    calculateSpectrum();
    calculateBands();
    smoothBands();
}

void SpectrumAnalyzer::calculateSpectrum()
{
    /*
     * Simple DFT for the desktop prototype.
     *
     * IMPORTANT:
     * This is intentionally simple for Milestone 8.
     * We should replace this with an actual FFT before
     * running it on the ESP32-S3.
     */

    for (size_t k = 0; k < FFT_SIZE / 2; ++k)
    {
        float real = 0.0f;
        float imag = 0.0f;

        for (size_t n = 0; n < FFT_SIZE; ++n)
        {
            // Hann window.
            const float window =
                0.5f *
                (1.0f -
                 std::cos(
                     2.0f * PI *
                     static_cast<float>(n) /
                     static_cast<float>(FFT_SIZE - 1)));

            const float sample =
                sampleBuffer[n] * window;

            const float angle =
                2.0f * PI *
                static_cast<float>(k) *
                static_cast<float>(n) /
                static_cast<float>(FFT_SIZE);

            real += sample * std::cos(angle);
            imag -= sample * std::sin(angle);
        }

        const float magnitude =
            std::sqrt(
                real * real +
                imag * imag);

        spectrum[k] =
            magnitude /
            static_cast<float>(FFT_SIZE);
    }
}

void SpectrumAnalyzer::calculateBands()
{
    /*
     * Divide the frequency spectrum into 16 bands.
     *
     * Low frequencies -> left
     * High frequencies -> right
     */

    constexpr size_t NUM_BINS =
        FFT_SIZE / 2;

    for (size_t bar = 0; bar < NUM_BARS; ++bar)
    {
        const size_t startBin =
            (bar * NUM_BINS) / NUM_BARS;

        const size_t endBin =
            ((bar + 1) * NUM_BINS) / NUM_BARS;

        float sum = 0.0f;
        size_t count = 0;

        for (size_t bin = startBin;
             bin < endBin;
             ++bin)
        {
            sum += spectrum[bin];
            ++count;
        }

        if (count > 0)
        {
            targetMagnitudes[bar] =
                sum /
                static_cast<float>(count);
        }
        else
        {
            targetMagnitudes[bar] = 0.0f;
        }

        /*
         * Increase the visual scale.
         *
         * The raw DFT magnitude is very small, so this
         * makes the bars visible.
         */
        targetMagnitudes[bar] *= 20.0f;

        targetMagnitudes[bar] =
            std::clamp(
                targetMagnitudes[bar],
                0.0f,
                1.0f);
    }
}

void SpectrumAnalyzer::smoothBands()
{
    for (size_t i = 0; i < NUM_BARS; ++i)
    {
        const float target =
            targetMagnitudes[i];

        const float current =
            magnitudes[i];

        if (target > current)
        {
            // Fast attack.
            magnitudes[i] =
                current +
                (target - current) *
                ATTACK;
        }
        else
        {
            // Slow decay.
            magnitudes[i] =
                current +
                (target - current) *
                DECAY;
        }
    }
}

float SpectrumAnalyzer::magnitudeForBin(
    size_t bin) const
{
    if (bin >= spectrum.size())
        return 0.0f;

    return spectrum[bin];
}

void SpectrumAnalyzer::render(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds)
{
    if (renderer == nullptr)
        return;

    constexpr float BAR_GAP = 6.0f;

    const float barWidth =
        (bounds.w -
         (NUM_BARS - 1) * BAR_GAP) /
        NUM_BARS;

    for (size_t i = 0; i < NUM_BARS; ++i)
    {
        const float magnitude =
            magnitudes[i];

        const float barHeight =
            magnitude * bounds.h;

        SDL_FRect bar{
            bounds.x +
                static_cast<float>(i) *
                (barWidth + BAR_GAP),

            bounds.y +
                bounds.h -
                barHeight,

            barWidth,
            barHeight
        };

        SDL_SetRenderDrawColor(
            renderer,
            170,
            220,
            170,
            255);

        SDL_RenderFillRect(
            renderer,
            &bar);
    }
}