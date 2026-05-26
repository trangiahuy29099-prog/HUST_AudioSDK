#pragma once

#include <AL/al.h>

#include <string>
#include <cstddef>

#include "Vector3.h"

class AudioEnvironment;

class AudioSource
{
private:
    ALuint sourceId;
    ALuint bufferId;

    ALuint occlusionFilter;
    bool occlusionFilterCreated;

public:
    AudioSource();
    ~AudioSource();

    AudioSource(const AudioSource &) = delete;
    AudioSource &operator=(const AudioSource &) = delete;

    bool LoadWav(const std::string &filepath);
    bool LoadWavFromMemory(const unsigned char *data, std::size_t dataSize);

    void Play();
    void Pause();
    void Stop();

    bool IsPlaying() const;

    void SetLooping(bool loop);

    void SetPosition(const Vector3 &position);
    void SetVelocity(const Vector3 &velocity);

    void SetPosition(float x, float y, float z);
    void SetVelocity(float x, float y, float z);

    void SetGain(float gain);
    void SetPitch(float pitch);

    void SetReferenceDistance(float distance);
    void SetMaxDistance(float distance);
    void SetRolloffFactor(float factor);

    void AttachEnvironment(const AudioEnvironment &environment);
    void DetachEnvironment();

    void SetOcclusion(float amount);

    ALuint GetSourceId() const;
};