#pragma once

#include <cstddef>

class IAudioSource
{
public:
    virtual ~IAudioSource() = default;

    virtual bool initialize() = 0;

    virtual size_t readSamples(
        float* buffer,
        size_t count) = 0;

    virtual bool isPlaying() const = 0;
};