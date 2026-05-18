#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

class AudioEngine
{
private:
    ALCdevice *device;
    ALCcontext *context;

    bool initialized;
    bool hrtfEnabled;

public:
    AudioEngine();
    ~AudioEngine();

    bool Init(bool enableHRTF = true);
    void Shutdown();

    bool IsInitialized() const;
    bool IsHRTFEnabled() const;

    void SetDistanceModel(ALenum model = AL_INVERSE_DISTANCE_CLAMPED);
    void SetDopplerFactor(float factor, float velocityOfSound = 343.3f);

    static bool CheckALError(const char *operation);
};