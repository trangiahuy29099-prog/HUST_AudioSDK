#pragma once
#include <AL/al.h>
#include <string>

class AudioSource {
private:
    ALuint sourceId;
    ALuint bufferId;

public:
    AudioSource();
    ~AudioSource();
    bool LoadWav(const std::string& filepath);
    void Play();
    void SetPosition(float x, float y, float z);
    void SetVelocity(float vx, float vy, float vz);
    void SetLooping(bool loop);
};
