#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>

class AudioEngine {
private:
    ALCdevice* device;
    ALCcontext* context;

public:
    AudioEngine();
    ~AudioEngine();
    void Init();
    void SetDopplerFactor(float factor, float velocityOfSound = 343.3f);
};
