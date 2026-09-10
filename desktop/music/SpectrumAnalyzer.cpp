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
    constexpr size_t NUM_BINS =
        FFT_SIZE / 2;

    /*
     * Human hearing is approximately logarithmic,
     * so use logarithmically spaced frequency bands
     * instead of evenly spaced FFT bins.
     *
     * This gives the visualizer much better balance
     * between bass, mids, and treble.
     */

    constexpr float SAMPLE_RATE = 44100.0f;

    constexpr float MIN_FREQUENCY = 40.0f;
    constexpr float MAX_FREQUENCY = 16000.0f;

    for (size_t bar = 0;
         bar < NUM_BARS;
         ++bar)
    {
        /*
         * Logarithmic frequency boundaries.
         */

        const float startRatio =
            static_cast<float>(bar) /
            static_cast<float>(NUM_BARS);

        const float endRatio =
            static_cast<float>(bar + 1) /
            static_cast<float>(NUM_BARS);

        const float startFrequency =
            MIN_FREQUENCY *
            std::pow(
                MAX_FREQUENCY / MIN_FREQUENCY,
                startRatio);

        const float endFrequency =
            MIN_FREQUENCY *
            std::pow(
                MAX_FREQUENCY / MIN_FREQUENCY,
                endRatio);

        /*
         * Convert frequencies into FFT bins.
         */

        size_t startBin =
            static_cast<size_t>(
                startFrequency *
                FFT_SIZE /
                SAMPLE_RATE);

        size_t endBin =
            static_cast<size_t>(
                endFrequency *
                FFT_SIZE /
                SAMPLE_RATE);

        /*
         * Clamp to valid FFT range.
         */

        startBin =
            std::clamp(
                startBin,
                size_t(0),
                NUM_BINS - 1);

        endBin =
            std::clamp(
                endBin,
                startBin + 1,
                NUM_BINS);

        /*
         * Average the magnitude in this band.
         */

        float sum = 0.0f;
        size_t binCount = 0;

        for (size_t bin = startBin;
             bin < endBin;
             ++bin)
        {
            sum += spectrum[bin];
            ++binCount;
        }

        float magnitude = 0.0f;

        if (binCount > 0)
        {
            magnitude =
                sum /
                static_cast<float>(binCount);
        }

        /*
         * Convert the raw FFT magnitude into a
         * more useful visual range.
         *
         * Higher frequencies naturally have less
         * energy, so apply a small compensation.
         */

        const float frequencyBoost =
            1.0f +
            static_cast<float>(bar) /
            static_cast<float>(NUM_BARS) *
            0.8f;

        magnitude *=
            20.0f *
            frequencyBoost;

        targetMagnitudes[bar] =
            std::clamp(
                magnitude,
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

    constexpr float BAR_GAP = 3.0f;

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