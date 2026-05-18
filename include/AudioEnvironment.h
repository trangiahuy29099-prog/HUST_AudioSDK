#pragma once

#include <AL/al.h>

enum class ReverbPreset
{
    None,
    SmallRoom,
    Cave,
    ConcertHall
};

class AudioEnvironment
{
private:
    ALuint effect;
    ALuint slot;
    bool ready;

public:
    AudioEnvironment();
    ~AudioEnvironment();

    AudioEnvironment(const AudioEnvironment &) = delete;
    AudioEnvironment &operator=(const AudioEnvironment &) = delete;

    bool Init(ReverbPreset preset);
    void Destroy();

    bool ApplyPreset(ReverbPreset preset);

    ALuint GetSlot() const;
    bool IsReady() const;
};