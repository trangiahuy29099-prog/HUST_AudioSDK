#include "AudioEngine.h"

#include <iostream>

AudioEngine::AudioEngine()
    : device(nullptr),
      context(nullptr),
      initialized(false),
      hrtfEnabled(false) {}

AudioEngine::~AudioEngine()
{
    Shutdown();
}

bool AudioEngine::Init(bool enableHRTF)
{
    device = alcOpenDevice(nullptr);

    if (!device)
    {
        std::cerr << "[AudioEngine] Failed to open default audio device." << std::endl;
        return false;
    }

    if (enableHRTF)
    {
        ALCint attrs[] = {
            ALC_HRTF_SOFT, ALC_TRUE,
            0};

        context = alcCreateContext(device, attrs);
    }
    else
    {
        context = alcCreateContext(device, nullptr);
    }

    if (!context)
    {
        std::cerr << "[AudioEngine] Failed to create OpenAL context." << std::endl;
        alcCloseDevice(device);
        device = nullptr;
        return false;
    }

    if (!alcMakeContextCurrent(context))
    {
        std::cerr << "[AudioEngine] Failed to make OpenAL context current." << std::endl;
        alcDestroyContext(context);
        alcCloseDevice(device);

        context = nullptr;
        device = nullptr;

        return false;
    }

    initialized = true;

    ALCint hrtfState = ALC_FALSE;
    alcGetIntegerv(device, ALC_HRTF_SOFT, 1, &hrtfState);

    hrtfEnabled = (hrtfState == ALC_TRUE);

    if (hrtfEnabled)
    {
        std::cout << "[AudioEngine] HRTF enabled." << std::endl;
        std::cout << "[AudioEngine] Use headphones for 360-degree spatial audio." << std::endl;
    }
    else
    {
        std::cout << "[AudioEngine] Warning: HRTF is not enabled or not supported." << std::endl;
    }

    SetDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    SetDopplerFactor(1.0f, 343.3f);

    CheckALError("AudioEngine::Init");

    return true;
}

void AudioEngine::Shutdown()
{
    if (context)
    {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        context = nullptr;
    }

    if (device)
    {
        alcCloseDevice(device);
        device = nullptr;
    }

    initialized = false;
    hrtfEnabled = false;
}

bool AudioEngine::IsInitialized() const
{
    return initialized;
}

bool AudioEngine::IsHRTFEnabled() const
{
    return hrtfEnabled;
}

void AudioEngine::SetDistanceModel(ALenum model)
{
    alDistanceModel(model);
    CheckALError("AudioEngine::SetDistanceModel");
}

void AudioEngine::SetDopplerFactor(float factor, float velocityOfSound)
{
    alDopplerFactor(factor);
    alSpeedOfSound(velocityOfSound);

    CheckALError("AudioEngine::SetDopplerFactor");
}

bool AudioEngine::CheckALError(const char *operation)
{
    ALenum error = alGetError();

    if (error != AL_NO_ERROR)
    {
        std::cerr << "[OpenAL Error] "
                  << operation
                  << " failed. Error code: "
                  << error
                  << std::endl;

        return false;
    }

    return true;
}