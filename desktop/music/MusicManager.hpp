#pragma once

#include <string>
#include <SDL3/SDL.h>

#include "MusicRenderer.hpp"

class MusicManager
{
public:
    MusicManager() = default;
    ~MusicManager() = default;

    bool initialize(SDL_Renderer* renderer);

    void update(float deltaTime);

    void render(
        SDL_Renderer* renderer,
        const SDL_FRect& bounds);

    void handleTouch(
        float x,
        float y,
        const SDL_FRect& bounds);

    void handleMouseClick(
        float x,
        float y,
        const SDL_FRect& bounds);

    void togglePlayPause();

    bool isInitialized() const;

    MusicPlayer& getMusicPlayer();
    const MusicPlayer& getMusicPlayer() const;

    void toggleMusicBox(
        const std::string& songPath);

    void stopMusicBox();

    bool isMusicBoxPlaying() const;

    void setLooping(bool enabled);

    void playSound(
        const std::string& path,
        const std::string& resumeMusicBoxPath);

private:
    MusicRenderer renderer;

    MusicPlayer musicBoxPlayer;

    bool initialized = false;

    bool musicBoxPlaying = false;

    bool alarmPlaying = false;
    float alarmTimer = 0.0f;

    bool resumeMusicPlayerAfterAlarm = false;
    bool resumeMusicBoxAfterAlarm = false;

    std::string resumeMusicBoxPath;
};