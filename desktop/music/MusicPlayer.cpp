#include "MusicPlayer.hpp"

#include <algorithm>
#include <cstring>

MusicPlayer::~MusicPlayer()
{
    destroyAudioStream();
}

bool MusicPlayer::initialize()
{
    /*
     * SDL3 audio is initialized through SDL_INIT_AUDIO.
     *
     * main.cpp already initializes SDL through SDL3Display,
     * but explicitly ensure the audio subsystem exists.
     */
    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            SDL_Log(
                "Failed to initialize SDL audio: %s",
                SDL_GetError());

            return false;
        }
    }

    return true;
}

bool MusicPlayer::load(
    const char* path)
{
    if (path == nullptr)
        return false;

    /*
     * Stop anything currently playing.
     */
    stop();

    audioData.clear();
    monoSamples.clear();

    SDL_AudioSpec spec{};

    Uint8* loadedData = nullptr;
    Uint32 loadedLength = 0;

    if (!SDL_LoadWAV(
            path,
            &spec,
            &loadedData,
            &loadedLength))
    {
        SDL_Log(
            "Failed to load WAV '%s': %s",
            path,
            SDL_GetError());

        return false;
    }

    /*
     * Copy the WAV data into our own storage.
     */
    audioData.resize(loadedLength);

    std::memcpy(
        audioData.data(),
        loadedData,
        loadedLength);

    SDL_free(loadedData);

    sourceSpec = spec;

    sampleRate =
        static_cast<size_t>(sourceSpec.freq);

    channelCount =
        static_cast<size_t>(sourceSpec.channels);

    /*
     * Convert the source to mono floating-point samples
     * for the spectrum analyzer.
     */
    convertToMono(
        audioData,
        sourceSpec);

    if (monoSamples.empty())
    {
        SDL_Log(
            "WAV contains no usable samples");

        audioData.clear();

        return false;
    }

    duration =
        static_cast<float>(monoSamples.size()) /
        static_cast<float>(sampleRate);

    playbackPosition = 0;
    currentTime = 0.0f;

    if (!createAudioStream(sourceSpec))
    {
        audioData.clear();
        monoSamples.clear();

        return false;
    }

    loaded = true;

    return true;
}

bool MusicPlayer::createAudioStream(
    const SDL_AudioSpec& sourceSpec)
{
    destroyAudioStream();

    /*
     * SDL_AudioStream lets us provide the WAV's original
     * format while SDL handles conversion to the playback
     * device format.
     */
    audioStream =
        SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &sourceSpec,
            nullptr,
            nullptr);

    if (audioStream == nullptr)
    {
        SDL_Log(
            "Failed to create audio stream: %s",
            SDL_GetError());

        return false;
    }

    return true;
}

void MusicPlayer::destroyAudioStream()
{
    if (audioStream != nullptr)
    {
        SDL_DestroyAudioStream(audioStream);
        audioStream = nullptr;
    }
}

void MusicPlayer::update()
{
    if (!loaded)
        return;

    if (!playing)
        return;

    if (audioStream == nullptr)
        return;

    fillAudioStream();

    /*
     * The stream is playing asynchronously.
     *
     * We use our source position to maintain the logical
     * playback position for the UI.
     */
    currentTime =
        static_cast<float>(playbackPosition) /
        static_cast<float>(sampleRate);

    if (currentTime >= duration)
    {
        stop();
    }
}

void MusicPlayer::fillAudioStream()
{
    if (playbackPosition >= audioData.size())
        return;

    /*
     * Keep approximately 100 ms of source audio queued.
     *
     * For the first implementation, calculate this using
     * the source format.
     */
    const size_t bytesPerSample =
        SDL_AUDIO_BITSIZE(sourceSpec.format) / 8;

    const size_t bytesPerFrame =
        bytesPerSample *
        static_cast<size_t>(sourceSpec.channels);

    if (bytesPerFrame == 0)
        return;

    const size_t targetBytes =
        static_cast<size_t>(
            sourceSpec.freq *
            bytesPerFrame *
            0.10f);

    const size_t queued =
        static_cast<size_t>(
            SDL_GetAudioStreamQueued(
                audioStream));

    if (queued >= targetBytes)
        return;

    const size_t remaining =
        audioData.size() -
        playbackPosition;

    const size_t bytesToQueue =
        std::min(
            remaining,
            targetBytes - queued);

    if (bytesToQueue == 0)
        return;

    if (!SDL_PutAudioStreamData(
            audioStream,
            audioData.data() + playbackPosition,
            static_cast<int>(bytesToQueue)))
    {
        SDL_Log(
            "Failed to queue audio: %s",
            SDL_GetError());

        return;
    }

    playbackPosition += bytesToQueue;
}

void MusicPlayer::play()
{
    if (!loaded)
        return;

    if (audioStream == nullptr)
        return;

    /*
     * If we reached the end, restart.
     */
    if (playbackPosition >= audioData.size())
    {
        playbackPosition = 0;
        currentTime = 0.0f;

        SDL_ClearAudioStream(audioStream);
    }

    playing = true;

    SDL_ResumeAudioStreamDevice(
        audioStream);
}

void MusicPlayer::pause()
{
    if (audioStream == nullptr)
        return;

    playing = false;

    SDL_PauseAudioStreamDevice(
        audioStream);
}

void MusicPlayer::togglePlayPause()
{
    if (playing)
        pause();
    else
        play();
}

void MusicPlayer::stop()
{
    playing = false;

    playbackPosition = 0;
    currentTime = 0.0f;

    if (audioStream != nullptr)
    {
        SDL_PauseAudioStreamDevice(
            audioStream);

        SDL_ClearAudioStream(
            audioStream);
    }
}

void MusicPlayer::seek(float time)
{
    if (!loaded)
        return;

    time = std::clamp(
        time,
        0.0f,
        duration);

    playbackPosition =
        static_cast<size_t>(
            time *
            static_cast<float>(sampleRate));

    currentTime = time;

    if (audioStream != nullptr)
    {
        SDL_ClearAudioStream(audioStream);

        if (playing)
        {
            SDL_ResumeAudioStreamDevice(
                audioStream);
        }
    }
}

bool MusicPlayer::isPlaying() const
{
    return playing;
}

bool MusicPlayer::isLoaded() const
{
    return loaded;
}

const float* MusicPlayer::getSamples() const
{
    if (monoSamples.empty())
        return nullptr;

    return monoSamples.data();
}

size_t MusicPlayer::getSampleCount() const
{
    return monoSamples.size();
}

float MusicPlayer::getCurrentTime() const
{
    return currentTime;
}

float MusicPlayer::getDuration() const
{
    return duration;
}

void MusicPlayer::convertToMono(
    const std::vector<Uint8>& sourceData,
    const SDL_AudioSpec& sourceSpec)
{
    monoSamples.clear();

    if (sourceData.empty())
        return;

    /*
     * This first backend supports the common WAV formats:
     *
     *   S16
     *   F32
     *
     * Stereo is averaged into mono.
     */
    const size_t channels =
        static_cast<size_t>(
            sourceSpec.channels);

    if (channels == 0)
        return;

    if (sourceSpec.format == SDL_AUDIO_S16)
    {
        const size_t totalSamples =
            sourceData.size() /
            sizeof(Sint16);

        const Sint16* samples =
            reinterpret_cast<const Sint16*>(
                sourceData.data());

        const size_t frames =
            totalSamples / channels;

        monoSamples.resize(frames);

        for (size_t frame = 0;
             frame < frames;
             ++frame)
        {
            float sum = 0.0f;

            for (size_t channel = 0;
                 channel < channels;
                 ++channel)
            {
                const Sint16 sample =
                    samples[
                        frame * channels +
                        channel];

                sum +=
                    static_cast<float>(sample) /
                    32768.0f;
            }

            monoSamples[frame] =
                sum /
                static_cast<float>(channels);
        }
    }
    else if (sourceSpec.format == SDL_AUDIO_F32)
    {
        const size_t totalSamples =
            sourceData.size() /
            sizeof(float);

        const float* samples =
            reinterpret_cast<const float*>(
                sourceData.data());

        const size_t frames =
            totalSamples / channels;

        monoSamples.resize(frames);

        for (size_t frame = 0;
             frame < frames;
             ++frame)
        {
            float sum = 0.0f;

            for (size_t channel = 0;
                 channel < channels;
                 ++channel)
            {
                sum +=
                    samples[
                        frame * channels +
                        channel];
            }

            monoSamples[frame] =
                sum /
                static_cast<float>(channels);
        }
    }
    else
    {
        SDL_Log(
            "Unsupported WAV format");

        monoSamples.clear();
    }
}