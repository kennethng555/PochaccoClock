#include "MusicManager.hpp"

bool MusicManager::initialize(
    SDL_Renderer* sdlRenderer)
{
    if (sdlRenderer == nullptr)
        return false;

    /*
     * Initialize the full Music Player.
     */
    if (!renderer.initialize(
            sdlRenderer,
            "../assets/fonts/PinkyBlues.otf"))
    {
        SDL_Log(
            "Failed to initialize MusicRenderer");

        return false;
    }

    /*
     * Initialize the independent Music Box player.
     */
    if (!musicBoxPlayer.initialize())
    {
        SDL_Log(
            "Failed to initialize Music Box player");

        return false;
    }

    initialized = true;
    musicBoxPlaying = false;

    return true;
}

void MusicManager::update()
{
    if (!initialized)
        return;

    /*
     * Update the full Music Player.
     */
    renderer.update();

    /*
     * Update the independent Music Box player.
     */
    musicBoxPlayer.update();

    if (musicBoxPlaying &&
        !musicBoxPlayer.isPlaying())
    {
        musicBoxPlayer.play();
    }
}

void MusicManager::render(
    SDL_Renderer* sdlRenderer,
    const SDL_FRect& bounds)
{
    if (!initialized)
        return;

    renderer.render(
        sdlRenderer,
        bounds);
}

void MusicManager::handleTouch(
    float x,
    float y,
    const SDL_FRect& bounds)
{
    if (!initialized)
        return;

    const MusicAction action =
        renderer.getAction(
            x,
            y,
            bounds);

    switch (action)
    {
        case MusicAction::PlayPause:
            togglePlayPause();
            return;

        case MusicAction::Previous:
        {
            MusicPlayer& player =
                renderer.getMusicPlayer();

            /*
             * Do not allow the full Music Player
             * to start while Music Box is active.
             */
            if (musicBoxPlaying)
                return;

            player.previous();
            return;
        }

        case MusicAction::Next:
        {
            MusicPlayer& player =
                renderer.getMusicPlayer();

            if (musicBoxPlaying)
                return;

            player.next();
            return;
        }

        case MusicAction::ToggleLoop:
        {
            MusicPlayer& player =
                renderer.getMusicPlayer();

            if (musicBoxPlaying)
                return;

            player.toggleLooping();
            return;
        }

        case MusicAction::ToggleShuffle:
        {
            MusicPlayer& player =
                renderer.getMusicPlayer();

            if (musicBoxPlaying)
                return;

            player.toggleShuffling();
            return;
        }

        case MusicAction::None:
        default:
            break;
    }

    /*
     * No music action was clicked.
     * Let MusicRenderer handle things such as
     * seeking on the progress bar.
     */
    renderer.handleTouch(
        x,
        y,
        bounds);
}

void MusicManager::handleMouseClick(
    float x,
    float y,
    const SDL_FRect& bounds)
{
    handleTouch(
        x,
        y,
        bounds);
}

void MusicManager::togglePlayPause()
{
    if (!initialized)
        return;

    MusicPlayer& player =
        renderer.getMusicPlayer();

    /*
     * If the normal Music Player is already playing,
     * pause it.
     */
    if (player.isPlaying())
    {
        player.pause();
        return;
    }

    /*
     * Normal Music Player is about to start.
     * Stop the Music Box first.
     */
    SDL_Log("musicBoxPlaying: %s", musicBoxPlaying ? "true" : "false");
    if (musicBoxPlaying)
    {
        musicBoxPlayer.stop();
        musicBoxPlaying = false;
    }

    /*
     * Now start/resume the normal Music Player.
     */
    player.play();
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
     * Toggle OFF.
     */
    if (musicBoxPlaying)
    {
        musicBoxPlayer.stop();
        musicBoxPlaying = false;
        return;
    }

    if (songPath.empty())
    {
        SDL_Log(
            "MusicManager: Music Box song path is empty");

        return;
    }

    MusicPlayer& player =
        renderer.getMusicPlayer();

    /*
     * Make sure the normal Music Player
     * cannot play at the same time.
     */
    if (player.isPlaying())
    {
        player.pause();
    }

    /*
     * Start Music Box.
     */
    musicBoxPlayer.stop();

    if (!musicBoxPlayer.load(
            songPath.c_str()))
    {
        SDL_Log(
            "MusicManager: failed to load Music Box song: %s",
            songPath.c_str());

        return;
    }

    musicBoxPlayer.play();

    musicBoxPlaying = true;
}

void MusicManager::stopMusicBox()
{
    if (!initialized)
        return;

    if (!musicBoxPlaying)
        return;

    musicBoxPlayer.stop();

    musicBoxPlaying = false;
}

bool MusicManager::isMusicBoxPlaying() const
{
    return musicBoxPlaying;
}