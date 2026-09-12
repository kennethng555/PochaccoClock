#include "MusicPlayer.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <random>

MusicPlayer::~MusicPlayer()
{
    destroyAudioStream();
}

bool MusicPlayer::initialize()
{
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

    stop();

    filename = std::filesystem::path(path).stem().string();

    audioData.clear();
    monoSamples.clear();

    title.clear();
    artist.clear();
    album.clear();
    genre.clear();

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

    audioData.resize(loadedLength);

    std::memcpy(
        audioData.data(),
        loadedData,
        loadedLength);

    SDL_free(loadedData);

    sourceSpec = spec;

    /*
     * Read WAV metadata before processing the
     * audio samples.
     */
    parseWavMetadata(audioData);

    sampleRate =
        static_cast<size_t>(sourceSpec.freq);

    channelCount =
        static_cast<size_t>(sourceSpec.channels);

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

    if (sampleRate == 0)
    {
        SDL_Log(
            "Invalid WAV sample rate");

        audioData.clear();
        monoSamples.clear();

        title.clear();
        artist.clear();
        album.clear();
        genre.clear();

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

    SDL_Log(
        "Loaded track: %s",
        path);

    SDL_Log(
        "Title: %s",
        title.empty()
            ? filename.c_str()
            : title.c_str());

    SDL_Log(
        "Artist: %s",
        artist.empty()
            ? "Unknown"
            : artist.c_str());

    return true;
}

bool MusicPlayer::loadDirectory(
    const char* directory)
{
    if (directory == nullptr)
        return false;

    musicFiles.clear();
    currentTrackIndex = 0;
    shuffledTracks.clear();
    shufflePosition = 0;
    shuffling = false;

    try
    {
        for (const auto& entry :
             std::filesystem::directory_iterator(
                 directory))
        {
            if (!entry.is_regular_file())
                continue;

            const std::string extension =
                entry.path().extension().string();

            /*
             * Only add WAV files.
             */
            if (extension == ".wav" ||
                extension == ".WAV")
            {
                musicFiles.push_back(
                    entry.path().string());
            }
        }
    }
    catch (
        const std::filesystem::filesystem_error& e)
    {
        SDL_Log(
            "Failed to scan music directory '%s': %s",
            directory,
            e.what());

        return false;
    }

    if (musicFiles.empty())
    {
        SDL_Log(
            "No WAV files found in '%s'",
            directory);

        return false;
    }

    /*
     * Keep the playlist order deterministic.
     */
    std::sort(
        musicFiles.begin(),
        musicFiles.end());

    SDL_Log(
        "Found %zu music tracks",
        musicFiles.size());

    for (size_t i = 0;
         i < musicFiles.size();
         ++i)
    {
        SDL_Log(
            "Track %zu: %s",
            i + 1,
            musicFiles[i].c_str());
    }

    return loadCurrentTrack();
}

bool MusicPlayer::loadCurrentTrack()
{
    if (musicFiles.empty())
        return false;

    if (currentTrackIndex >= musicFiles.size())
        currentTrackIndex = 0;

    return load(
        musicFiles[currentTrackIndex].c_str());
}

void MusicPlayer::next()
{
    if (musicFiles.empty())
        return;

    if (shuffling)
    {
        if (shuffledTracks.empty())
        {
            createShuffleOrder();
        }

        /*
         * Move to the next position in the
         * current shuffled playback order.
         */
        ++shufflePosition;

        /*
         * End of the shuffled order.
         * Create a new order and skip the
         * current track, which is always at
         * position 0.
         */
        if (shufflePosition >= shuffledTracks.size())
        {
            createShuffleOrder();

            if (shuffledTracks.size() > 1)
            {
                shufflePosition = 1;
            }
            else
            {
                shufflePosition = 0;
            }
        }

        currentTrackIndex =
            shuffledTracks[shufflePosition];
    }
    else
    {
        /*
         * Normal sequential playback.
         */
        currentTrackIndex =
            (currentTrackIndex + 1) %
            musicFiles.size();
    }

    SDL_Log(
        "Next track: %zu / %zu",
        currentTrackIndex + 1,
        musicFiles.size());

    if (loadCurrentTrack())
    {
        play();
    }
}

void MusicPlayer::previous()
{
    if (musicFiles.empty())
        return;

    if (shuffling)
    {
        if (shuffledTracks.empty())
        {
            createShuffleOrder();
        }

        /*
         * Move backward through the current
         * shuffled playback order.
         */
        if (shufflePosition == 0)
        {
            /*
             * Wrap to the last track in the
             * current shuffled order.
             */
            shufflePosition =
                shuffledTracks.size() - 1;
        }
        else
        {
            --shufflePosition;
        }

        currentTrackIndex =
            shuffledTracks[shufflePosition];
    }
    else
    {
        /*
         * Normal sequential playback.
         */
        if (currentTrackIndex == 0)
        {
            currentTrackIndex =
                musicFiles.size() - 1;
        }
        else
        {
            --currentTrackIndex;
        }
    }

    SDL_Log(
        "Previous track: %zu / %zu",
        currentTrackIndex + 1,
        musicFiles.size());

    if (loadCurrentTrack())
    {
        play();
    }
}

size_t MusicPlayer::getTrackIndex() const
{
    return currentTrackIndex;
}

size_t MusicPlayer::getTrackCount() const
{
    return musicFiles.size();
}

bool MusicPlayer::createAudioStream(
    const SDL_AudioSpec& sourceSpec)
{
    destroyAudioStream();

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

    /*
     * Keep the audio stream supplied with data.
     */
    fillAudioStream();

    /*
     * playbackPosition is the number of SOURCE BYTES
     * submitted to SDL.
     *
     * SDL_GetAudioStreamQueued() returns the number of
     * source bytes still waiting in the stream.
     *
     * submitted - queued = consumed.
     */
    const int queuedResult =
        SDL_GetAudioStreamQueued(
            audioStream);

    if (queuedResult < 0)
    {
        SDL_Log(
            "SDL_GetAudioStreamQueued failed: %s",
            SDL_GetError());

        return;
    }

    const size_t queuedBytes =
        static_cast<size_t>(queuedResult);

    const size_t bytesPerSample =
        SDL_AUDIO_BITSIZE(
            sourceSpec.format) / 8;

    const size_t bytesPerFrame =
        bytesPerSample *
        static_cast<size_t>(
            sourceSpec.channels);

    if (bytesPerFrame == 0 ||
        sampleRate == 0)
    {
        return;
    }

    const size_t consumedBytes =
        playbackPosition > queuedBytes
            ? playbackPosition - queuedBytes
            : 0;

    const size_t consumedFrames =
        consumedBytes / bytesPerFrame;

    currentTime =
        static_cast<float>(consumedFrames) /
        static_cast<float>(sampleRate);

    /*
     * The entire track has finished only when:
     *
     * 1. All source data has been submitted.
     * 2. SDL has consumed all queued data.
     */
    if (playbackPosition >= audioData.size() &&
        queuedBytes == 0)
    {
        currentTime = duration;

        playing = false;

        SDL_PauseAudioStreamDevice(
            audioStream);

        /*
         * No playlist.
         *
         * This is the Music Box case.
         * The MusicManager handles its looping.
         */
        if (musicFiles.empty())
        {
            return;
        }

        /*
         * Loop the CURRENT song.
         */
        if (looping)
        {
            SDL_Log(
                "Track finished. Looping track %zu / %zu",
                currentTrackIndex + 1,
                musicFiles.size());

            if (loadCurrentTrack())
            {
                play();
            }

            return;
        }

        /*
         * Shuffle.
         */
        if (shuffling)
        {
            /*
            * Advance within the shuffled playlist.
            */
            ++shufflePosition;

            /*
            * The shuffled playlist is exhausted.
            * Generate a new shuffled order.
            */
            if (shufflePosition >= shuffledTracks.size())
            {
                createShuffleOrder();

                /*
                * createShuffleOrder() puts the current
                * track first. Skip it so we don't play
                * the same song twice in a row.
                */
                if (shuffledTracks.size() > 1)
                {
                    shufflePosition = 1;
                }
                else
                {
                    /*
                    * Only one song exists.
                    * Replay it.
                    */
                    shufflePosition = 0;
                }
            }

            currentTrackIndex =
                shuffledTracks[shufflePosition];
        }
        else
        {
            /*
            * Normal sequential playback.
            */
            currentTrackIndex =
                (currentTrackIndex + 1) %
                musicFiles.size();
        }

        SDL_Log(
            "Track finished. Advancing to track %zu / %zu",
            currentTrackIndex + 1,
            musicFiles.size());

        if (loadCurrentTrack())
        {
            play();
        }
    }
}

void MusicPlayer::fillAudioStream()
{
    if (audioStream == nullptr)
        return;

    if (playbackPosition >= audioData.size())
        return;

    const size_t bytesPerSample =
        SDL_AUDIO_BITSIZE(
            sourceSpec.format) / 8;

    const size_t bytesPerFrame =
        bytesPerSample *
        static_cast<size_t>(
            sourceSpec.channels);

    if (bytesPerFrame == 0)
        return;

    /*
     * Keep approximately 100 ms of audio queued.
     */
    const size_t targetBytes =
        static_cast<size_t>(
            static_cast<float>(sourceSpec.freq) *
            static_cast<float>(bytesPerFrame) *
            0.10f);

    const int queuedResult =
        SDL_GetAudioStreamQueued(
            audioStream);

    if (queuedResult < 0)
    {
        SDL_Log(
            "SDL_GetAudioStreamQueued failed: %s",
            SDL_GetError());

        return;
    }

    const size_t queued =
        static_cast<size_t>(queuedResult);

    if (queued >= targetBytes)
        return;

    const size_t remaining =
        audioData.size() -
        playbackPosition;

    const size_t availableBytes =
        targetBytes -
        queued;

    /*
     * Only queue complete audio frames.
     */
    const size_t framesToQueue =
        std::min(
            remaining / bytesPerFrame,
            availableBytes / bytesPerFrame);

    const size_t bytesToQueue =
        framesToQueue *
        bytesPerFrame;

    if (bytesToQueue == 0)
        return;

    if (!SDL_PutAudioStreamData(
            audioStream,
            audioData.data() +
                playbackPosition,
            static_cast<int>(bytesToQueue)))
    {
        SDL_Log(
            "Failed to queue audio: %s",
            SDL_GetError());

        return;
    }

    /*
     * playbackPosition represents SOURCE DATA
     * submitted to SDL.
     */
    playbackPosition += bytesToQueue;
}

void MusicPlayer::play()
{
    if (!loaded)
        return;

    if (audioStream == nullptr)
        return;

    /*
     * If we're at the end, restart the current
     * track from the beginning.
     */
    if (playbackPosition >= audioData.size())
    {
        playbackPosition = 0;
        currentTime = 0.0f;

        SDL_ClearAudioStream(
            audioStream);
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

    const size_t bytesPerSample =
        SDL_AUDIO_BITSIZE(
            sourceSpec.format) / 8;

    const size_t bytesPerFrame =
        bytesPerSample *
        static_cast<size_t>(
            sourceSpec.channels);

    if (bytesPerFrame == 0 ||
        sampleRate == 0)
    {
        return;
    }

    /*
     * Convert time -> frame.
     */
    const size_t targetFrame =
        static_cast<size_t>(
            time *
            static_cast<float>(sampleRate));

    /*
     * Convert frame -> source bytes.
     */
    playbackPosition =
        std::min(
            targetFrame * bytesPerFrame,
            audioData.size());

    currentTime =
        static_cast<float>(targetFrame) /
        static_cast<float>(sampleRate);

    /*
     * Discard previously queued audio.
     */
    if (audioStream != nullptr)
    {
        SDL_ClearAudioStream(
            audioStream);

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

bool MusicPlayer::isLooping() const
{
    return looping;
}

void MusicPlayer::toggleLooping()
{
    looping = !looping;

    if (looping)
    {
        /*
         * Loop and shuffle are mutually exclusive.
         */
        shuffling = false;

        shuffledTracks.clear();
        shufflePosition = 0;
    }
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

const std::string& MusicPlayer::getFilename() const
{
    return filename;
}

const std::string& MusicPlayer::getTitle() const
{
    return title;
}

const std::string& MusicPlayer::getArtist() const
{
    return artist;
}

const std::string& MusicPlayer::getAlbum() const
{
    return album;
}

const std::string& MusicPlayer::getGenre() const
{
    return genre;
}

void MusicPlayer::convertToMono(
    const std::vector<Uint8>& sourceData,
    const SDL_AudioSpec& sourceSpec)
{
    monoSamples.clear();

    if (sourceData.empty())
        return;

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

void MusicPlayer::parseWavMetadata(
    const std::vector<Uint8>& data)
{
    title.clear();
    artist.clear();
    album.clear();
    genre.clear();

    if (data.size() < 12)
        return;

    if (std::memcmp(
            data.data(),
            "RIFF",
            4) != 0)
    {
        return;
    }

    if (std::memcmp(
            data.data() + 8,
            "WAVE",
            4) != 0)
    {
        return;
    }

    size_t offset = 12;

    while (offset + 8 <= data.size())
    {
        const char* chunkId =
            reinterpret_cast<const char*>(
                data.data() + offset);

        const Uint32 chunkSize =
            static_cast<Uint32>(
                data[offset + 4]) |
            (static_cast<Uint32>(
                data[offset + 5]) << 8) |
            (static_cast<Uint32>(
                data[offset + 6]) << 16) |
            (static_cast<Uint32>(
                data[offset + 7]) << 24);

        offset += 8;

        if (chunkSize >
            data.size() - offset)
        {
            break;
        }

        if (std::memcmp(
                chunkId,
                "LIST",
                4) == 0 &&
            chunkSize >= 4)
        {
            const char* listType =
                reinterpret_cast<const char*>(
                    data.data() + offset);

            if (std::memcmp(
                    listType,
                    "INFO",
                    4) == 0)
            {
                size_t infoOffset =
                    offset + 4;

                const size_t infoEnd =
                    offset + chunkSize;

                while (infoOffset + 8 <= infoEnd)
                {
                    const char* infoId =
                        reinterpret_cast<const char*>(
                            data.data() +
                            infoOffset);

                    const Uint32 infoSize =
                        static_cast<Uint32>(
                            data[infoOffset + 4]) |
                        (static_cast<Uint32>(
                            data[infoOffset + 5]) << 8) |
                        (static_cast<Uint32>(
                            data[infoOffset + 6]) << 16) |
                        (static_cast<Uint32>(
                            data[infoOffset + 7]) << 24);

                    infoOffset += 8;

                    if (infoSize >
                        infoEnd - infoOffset)
                    {
                        break;
                    }

                    std::string value;

                    if (infoSize > 0)
                    {
                        value.assign(
                            reinterpret_cast<const char*>(
                                data.data() +
                                infoOffset),
                            infoSize);

                        const size_t nullPos =
                            value.find('\0');

                        if (nullPos !=
                            std::string::npos)
                        {
                            value.resize(nullPos);
                        }

                        while (!value.empty() &&
                               (value.back() == ' ' ||
                                value.back() == '\t' ||
                                value.back() == '\r' ||
                                value.back() == '\n'))
                        {
                            value.pop_back();
                        }

                        size_t first = 0;

                        while (first < value.size() &&
                               (value[first] == ' ' ||
                                value[first] == '\t' ||
                                value[first] == '\r' ||
                                value[first] == '\n'))
                        {
                            ++first;
                        }

                        if (first > 0)
                        {
                            value.erase(
                                0,
                                first);
                        }
                    }

                    if (std::memcmp(
                            infoId,
                            "INAM",
                            4) == 0)
                    {
                        title = value;
                    }
                    else if (std::memcmp(
                                 infoId,
                                 "IART",
                                 4) == 0)
                    {
                        artist = value;
                    }
                    else if (std::memcmp(
                                 infoId,
                                 "IPRD",
                                 4) == 0)
                    {
                        album = value;
                    }
                    else if (std::memcmp(
                                 infoId,
                                 "IGNR",
                                 4) == 0)
                    {
                        genre = value;
                    }

                    infoOffset += infoSize;

                    if (infoSize & 1)
                        ++infoOffset;
                }
            }
        }

        offset += chunkSize;

        if (chunkSize & 1)
            ++offset;
    }

    SDL_Log(
        "WAV metadata: title='%s', artist='%s', album='%s', genre='%s'",
        title.c_str(),
        artist.c_str(),
        album.c_str(),
        genre.c_str());
}

void MusicPlayer::createShuffleOrder()
{
    shuffledTracks.clear();

    if (musicFiles.empty())
    {
        shufflePosition = 0;
        return;
    }

    /*
     * Put the currently playing track first.
     */
    shuffledTracks.push_back(
        currentTrackIndex);

    /*
     * Add every other track.
     */
    for (std::size_t i = 0;
         i < musicFiles.size();
         ++i)
    {
        if (i == currentTrackIndex)
            continue;

        shuffledTracks.push_back(i);
    }

    /*
     * Shuffle everything AFTER the current track.
     */
    if (shuffledTracks.size() > 2)
    {
        std::random_device rd;
        std::mt19937 generator(rd());

        std::shuffle(
            shuffledTracks.begin() + 1,
            shuffledTracks.end(),
            generator);
    }

    /*
     * The current track is at position 0.
     */
    shufflePosition = 0;
}

bool MusicPlayer::isShuffling() const
{
    return shuffling;
}

void MusicPlayer::toggleShuffling()
{
    shuffling = !shuffling;

    if (shuffling)
    {
        /*
         * Shuffle and loop are mutually exclusive.
         */
        looping = false;

        /*
         * Build a new playback order without
         * interrupting the current track.
         */
        createShuffleOrder();
    }
    else
    {
        shuffledTracks.clear();
        shufflePosition = 0;
    }
}