#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

class MusicPlayer
{
public:
    MusicPlayer() = default;
    ~MusicPlayer();

    bool initialize();

    bool load(const char* path);
    bool loadDirectory(const char* directory);

    void next();
    void previous();

    size_t getTrackIndex() const;
    size_t getTrackCount() const;

    void update();

    void play();
    void pause();
    void togglePlayPause();
    void stop();
    void seek(float time);

    bool isPlaying() const;
    bool isLoaded() const;
    bool isLooping() const;
    void toggleLooping();

    bool isShuffling() const;
    void toggleShuffling();

    const float* getSamples() const;
    size_t getSampleCount() const;

    float getCurrentTime() const;
    float getDuration() const;

    const std::string& getFilename() const;
    const std::string& getTitle() const;
    const std::string& getArtist() const;
    const std::string& getAlbum() const;
    const std::string& getGenre() const;

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
    
    size_t queuedPlaybackBytes = 0;

    size_t sampleRate = 44100;

    size_t channelCount = 2;

    bool loaded = false;
    bool playing = false;
    bool looping = false;
    bool shuffling = false;

    float currentTime = 0.0f;
    float duration = 0.0f;

    std::string filename;
    std::string title;
    std::string artist;
    std::string album;
    std::string genre;

    void parseWavMetadata(
        const std::vector<Uint8>& data);

    std::vector<std::string> musicFiles;
    size_t currentTrackIndex = 0;

    std::vector<std::size_t> shuffledTracks;
    std::size_t shufflePosition = 0;

    void createShuffleOrder();

    bool loadCurrentTrack();
};