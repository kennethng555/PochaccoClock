#include "MusicManager.hpp"

#include <SDL3/SDL.h>

bool MusicManager::initialize(SDL_Renderer* renderer)
{
    if (!renderer)
    {
        SDL_Log("MusicManager: renderer is null");
        return false;
    }

    if (!this->renderer.initialize(renderer, "../assets/fonts/PinkyBlues.otf"))
    {
        SDL_Log("MusicManager: failed to initialize MusicRenderer");
        return false;
    }

    initialized = true;

    musicBoxPlaying = false;
    alarmPlaying = false;
    alarmTimer = 0.0f;

    birthdayPlaying = false;
    birthdayMusicPath.clear();
    normalMusicBoxPath.clear();
    normalMusicBoxWasPlaying = false;

    return true;
}

void MusicManager::update(float deltaTime)
{
    if (!initialized)
        return;

    /*
     * Alarm playback takes priority over everything else.
     */
    if (alarmPlaying)
    {
        alarmTimer += deltaTime;

        if (alarmTimer >= 30.0f)
        {
            alarmPlaying = false;
            alarmTimer = 0.0f;

            musicBoxPlayer.stop();

            /*
             * Birthday music has priority over the normal Music Box
             * when birthday mode is still active.
             */
            if (birthdayPlaying)
            {
                if (!birthdayMusicPath.empty() &&
                    musicBoxPlayer.load(birthdayMusicPath.c_str()))
                {
                    musicBoxPlayer.setLooping(true);
                    musicBoxPlayer.play();
                }
            }
            else if (resumeMusicBoxAfterAlarm &&
                     !resumeMusicBoxPath.empty())
            {
                if (musicBoxPlayer.load(resumeMusicBoxPath.c_str()))
                {
                    musicBoxPlayer.setLooping(true);
                    musicBoxPlayer.play();
                    musicBoxPlaying = true;
                }
            }

            if (resumeMusicPlayerAfterAlarm)
            {
                renderer.getMusicPlayer().play();
            }

            resumeMusicPlayerAfterAlarm = false;
            resumeMusicBoxAfterAlarm = false;
            resumeMusicBoxPath.clear();
        }
    }

    /*
     * Update the normal Music Renderer.
     *
     * This is independent from the Music Box player.
     */
    renderer.update();

    /*
     * Do not update/play normal Music Box state while an alarm
     * or birthday song has priority.
     */
    if (!alarmPlaying && !birthdayPlaying)
    {
        if (musicBoxPlaying)
        {
            musicBoxPlayer.update();
        }
    }
}

void MusicManager::render(
    SDL_Renderer* renderer,
    const SDL_FRect& bounds)
{
    if (!initialized)
        return;

    this->renderer.render(renderer, bounds);
}

void MusicManager::handleTouch(
    float x,
    float y,
    const SDL_FRect& bounds)
{
    if (!initialized)
        return;

    this->renderer.handleTouch(x, y, bounds);
}

void MusicManager::handleMouseClick(
    float x,
    float y,
    const SDL_FRect& bounds)
{
    if (!initialized)
        return;

    this->renderer.handleMouseClick(x, y, bounds);
}

void MusicManager::togglePlayPause()
{
    if (!initialized)
        return;

    if (alarmPlaying)
        return;

    renderer.getMusicPlayer().togglePlayPause();
}

bool MusicManager::isInitialized() const
{
    return initialized;
}

MusicPlayer& MusicManager::getMusicPlayer()
{
    return renderer.getMusicPlayer();
}

const MusicPlayer& MusicManager::getMusicPlayer() const
{
    return renderer.getMusicPlayer();
}

void MusicManager::toggleMusicBox(
    const std::string& songPath)
{
    if (!initialized)
        return;

    /*
     * Birthday music owns the Music Box while active.
     */
    if (birthdayPlaying)
        return;

    if (alarmPlaying)
        return;

    if (musicBoxPlaying)
    {
        stopMusicBox();
        return;
    }

    if (songPath.empty())
    {
        SDL_Log(
            "MusicManager: Music Box song path is empty");

        return;
    }

    /*
     * Pause the normal music player while the
     * Music Box is active.
     */
    if (renderer.getMusicPlayer().isPlaying())
    {
        renderer.getMusicPlayer().pause();
    }

    musicBoxPlayer.stop();

    if (!musicBoxPlayer.load(songPath.c_str()))
    {
        SDL_Log(
            "MusicManager: failed to load Music Box song: %s",
            songPath.c_str());

        return;
    }

    musicBoxPlayer.setLooping(true);
    musicBoxPlayer.play();

    musicBoxPlaying = true;
}

void MusicManager::stopMusicBox()
{
    if (!initialized)
        return;

    musicBoxPlayer.stop();

    musicBoxPlaying = false;
}

bool MusicManager::isMusicBoxPlaying() const
{
    /*
     * The UI should consider the Music Box "on" while birthday
     * music is playing because birthday music occupies the same
     * Music Box playback channel.
     */
    return musicBoxPlaying || birthdayPlaying;
}

void MusicManager::setLooping(bool enabled)
{
    if (!initialized)
        return;

    musicBoxPlayer.setLooping(enabled);
}

void MusicManager::playSound(
    const std::string& path,
    const std::string& resumeMusicBoxPath)
{
    if (!initialized)
        return;

    if (path.empty())
    {
        SDL_Log("MusicManager: alarm sound path is empty");
        return;
    }

    if (alarmPlaying)
        return;

    /*
     * Save the current normal music state.
     */
    resumeMusicPlayerAfterAlarm =
        renderer.getMusicPlayer().isPlaying();

    /*
     * Save the current Music Box state.
     *
     * Birthday music is handled separately. If birthday mode is
     * active, the birthday song will automatically resume after
     * the alarm.
     */
    resumeMusicBoxAfterAlarm =
        musicBoxPlaying && !birthdayPlaying;

    this->resumeMusicBoxPath = resumeMusicBoxPath;

    /*
     * Pause normal music.
     */
    if (renderer.getMusicPlayer().isPlaying())
    {
        renderer.getMusicPlayer().pause();
    }

    /*
     * Stop the Music Box playback temporarily.
     */
    musicBoxPlayer.stop();

    musicBoxPlaying = false;

    /*
     * Alarm takes over the Music Box player.
     */
    if (!musicBoxPlayer.load(path.c_str()))
    {
        SDL_Log(
            "MusicManager: failed to load alarm sound: %s",
            path.c_str());

        resumeMusicPlayerAfterAlarm = false;
        resumeMusicBoxAfterAlarm = false;
        this->resumeMusicBoxPath.clear();

        return;
    }

    musicBoxPlayer.setLooping(true);
    musicBoxPlayer.play();

    alarmPlaying = true;
    alarmTimer = 0.0f;
}

void MusicManager::startBirthdayMusic(
    const std::string& birthdayPath,
    const std::string& normalMusicBoxPath)
{
    if (!initialized)
        return;

    if (alarmPlaying)
        return;

    if (birthdayPlaying)
        return;

    if (birthdayPath.empty())
    {
        SDL_Log("MusicManager: birthday music path is empty");
        return;
    }

    /*
     * Remember the normal Music Box configuration so it can be
     * restored when the birthday date ends.
     */
    this->normalMusicBoxPath = normalMusicBoxPath;
    this->normalMusicBoxWasPlaying = musicBoxPlaying;

    birthdayMusicPath = birthdayPath;

    /*
     * Stop the normal Music Box.
     */
    musicBoxPlayer.stop();
    musicBoxPlaying = false;

    /*
     * Pause normal music while the birthday song is playing.
     */
    if (renderer.getMusicPlayer().isPlaying())
    {
        renderer.getMusicPlayer().pause();
    }

    /*
     * Load the birthday song into the same Music Box player.
     */
    if (!musicBoxPlayer.load(birthdayPath.c_str()))
    {
        SDL_Log(
            "MusicManager: failed to load birthday music: %s",
            birthdayPath.c_str());

        birthdayMusicPath.clear();
        this->normalMusicBoxPath.clear();
        normalMusicBoxWasPlaying = false;

        return;
    }

    musicBoxPlayer.setLooping(true);
    musicBoxPlayer.play();

    birthdayPlaying = true;

    SDL_Log(
        "MusicManager: birthday music started: %s",
        birthdayPath.c_str());
}

void MusicManager::stopBirthdayMusic()
{
    if (!initialized)
        return;

    if (!birthdayPlaying)
        return;

    /*
     * Stop the birthday song.
     */
    musicBoxPlayer.stop();

    birthdayPlaying = false;

    /*
     * Restore the normal Music Box only if it was playing when
     * birthday mode began.
     */
    if (normalMusicBoxWasPlaying &&
        !normalMusicBoxPath.empty())
    {
        if (musicBoxPlayer.load(normalMusicBoxPath.c_str()))
        {
            musicBoxPlayer.setLooping(true);
            musicBoxPlayer.play();

            musicBoxPlaying = true;

            SDL_Log(
                "MusicManager: restored normal Music Box: %s",
                normalMusicBoxPath.c_str());
        }
        else
        {
            SDL_Log(
                "MusicManager: failed to restore normal Music Box: %s",
                normalMusicBoxPath.c_str());

            musicBoxPlaying = false;
        }
    }
    else
    {
        musicBoxPlaying = false;
    }

    birthdayMusicPath.clear();
    normalMusicBoxPath.clear();
    normalMusicBoxWasPlaying = false;
}

bool MusicManager::isBirthdayMusicPlaying() const
{
    return birthdayPlaying;
}