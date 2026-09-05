#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class MusicPlayer
{
public:
    MusicPlayer() = default;
    ~MusicPlayer();

    bool initialize();

    bool load(
        const char* path);

    void update();

    void play();
    void pause();
    void togglePlayPause();
    void stop();
    void seek(float time);

    bool isPlaying() const;

    bool isLoaded() const;

    const float* getSamples() const;
    size_t getSampleCount() const;

    float getCurrentTime() const;
    float getDuration() const;

private:
    bool createAudioStream(
        const SDL_AudioSpec& sourceSpec);

    void destroyAudioStream();

    void fillAudioStream();

    void convertToMono(
        const std::vector<Uint8>& sourceData,
        const SDL_AudioSpec& sourceSpec);

private:
    SDL_AudioStream* audioStream = nullptr;

    SDL_AudioSpec sourceSpec{};

    std::vector<Uint8> audioData;

    std::vector<float> monoSamples;

    size_t playbackPosition = 0;

    size_t sampleRate = 44100;

    size_t channelCount = 2;

    bool loaded = false;
    bool playing = false;

    float currentTime = 0.0f;
    float duration = 0.0f;
};