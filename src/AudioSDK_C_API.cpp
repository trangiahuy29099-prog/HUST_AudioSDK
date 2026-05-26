#include "AudioSDK_C_API.h"

#include "AudioEngine.h"
#include "AudioSource.h"
#include "AudioListener.h"
#include "AudioEnvironment.h"
#include "Vector3.h"

#include <iostream>

static AudioEngine *gEngine = nullptr;
static AudioSource *gSource = nullptr;
static AudioEnvironment *gCaveEnvironment = nullptr;

static bool gInitialized = false;
static bool gReverbReady = false;
static bool gReverbEnabled = false;

AUDIO_SDK_API int AudioSDK_Init()
{
    if (gInitialized)
    {
        return 1;
    }

    gEngine = new AudioEngine();

    if (!gEngine->Init(true))
    {
        std::cerr << "[AudioSDK_C_API] Failed to initialize AudioEngine." << std::endl;

        delete gEngine;
        gEngine = nullptr;

        return 0;
    }

    gEngine->SetDopplerFactor(1.4f, 343.3f);
    gEngine->SetDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

    AudioListener::SetPosition(Vector3(0.0f, 0.0f, 0.0f));
    AudioListener::SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
    AudioListener::SetOrientation(
        Vector3(0.0f, 0.0f, -1.0f),
        Vector3(0.0f, 1.0f, 0.0f));

    gSource = new AudioSource();

    gSource->SetGain(1.0f);
    gSource->SetPitch(1.0f);
    gSource->SetReferenceDistance(8.0f);
    gSource->SetMaxDistance(120.0f);
    gSource->SetRolloffFactor(0.65f);

    gCaveEnvironment = new AudioEnvironment();
    gReverbReady = gCaveEnvironment->Init(ReverbPreset::Cave);

    gInitialized = true;

    std::cout << "[AudioSDK_C_API] SDK initialized successfully." << std::endl;

    return 1;
}

AUDIO_SDK_API void AudioSDK_Shutdown()
{
    if (gSource)
    {
        gSource->Stop();
        delete gSource;
        gSource = nullptr;
    }

    if (gCaveEnvironment)
    {
        delete gCaveEnvironment;
        gCaveEnvironment = nullptr;
    }

    if (gEngine)
    {
        gEngine->Shutdown();
        delete gEngine;
        gEngine = nullptr;
    }

    gInitialized = false;
    gReverbReady = false;
    gReverbEnabled = false;

    std::cout << "[AudioSDK_C_API] SDK shutdown." << std::endl;
}

AUDIO_SDK_API int AudioSDK_LoadSound(const char *filepath)
{
    if (!gInitialized || !gSource || filepath == nullptr)
    {
        return 0;
    }

    bool ok = gSource->LoadWav(filepath);

    if (!ok)
    {
        std::cerr << "[AudioSDK_C_API] Failed to load sound: " << filepath << std::endl;
        return 0;
    }

    return 1;
}

AUDIO_SDK_API int AudioSDK_LoadSoundFromMemory(const unsigned char *data, int dataSize)
{
    if (!gInitialized || !gSource || data == nullptr || dataSize <= 0)
    {
        return 0;
    }

    bool ok = gSource->LoadWavFromMemory(
        data,
        static_cast<std::size_t>(dataSize));

    if (!ok)
    {
        std::cerr << "[AudioSDK_C_API] Failed to load sound from Unity memory buffer." << std::endl;
        return 0;
    }

    return 1;
}

AUDIO_SDK_API void AudioSDK_Play()
{
    if (!gSource)
    {
        return;
    }

    gSource->Play();
}

AUDIO_SDK_API void AudioSDK_Stop()
{
    if (!gSource)
    {
        return;
    }

    gSource->Stop();
}

AUDIO_SDK_API void AudioSDK_SetLooping(int loop)
{
    if (!gSource)
    {
        return;
    }

    gSource->SetLooping(loop != 0);
}

AUDIO_SDK_API void AudioSDK_SetSourcePosition(float x, float y, float z)
{
    if (!gSource)
    {
        return;
    }

    gSource->SetPosition(Vector3(x, y, z));
}

AUDIO_SDK_API void AudioSDK_SetSourceVelocity(float x, float y, float z)
{
    if (!gSource)
    {
        return;
    }

    gSource->SetVelocity(Vector3(x, y, z));
}

AUDIO_SDK_API void AudioSDK_SetListenerPosition(float x, float y, float z)
{
    AudioListener::SetPosition(Vector3(x, y, z));
}

AUDIO_SDK_API void AudioSDK_SetListenerVelocity(float x, float y, float z)
{
    AudioListener::SetVelocity(Vector3(x, y, z));
}

AUDIO_SDK_API void AudioSDK_SetListenerOrientation(
    float fx, float fy, float fz,
    float ux, float uy, float uz)
{
    AudioListener::SetOrientation(
        Vector3(fx, fy, fz),
        Vector3(ux, uy, uz));
}

AUDIO_SDK_API void AudioSDK_SetReverb(int enabled)
{
    if (!gSource || !gCaveEnvironment || !gReverbReady)
    {
        return;
    }

    bool shouldEnable = enabled != 0;

    if (shouldEnable && !gReverbEnabled)
    {
        gSource->AttachEnvironment(*gCaveEnvironment);
        gReverbEnabled = true;
        std::cout << "[AudioSDK_C_API] Reverb ON." << std::endl;
    }
    else if (!shouldEnable && gReverbEnabled)
    {
        gSource->DetachEnvironment();
        gReverbEnabled = false;
        std::cout << "[AudioSDK_C_API] Reverb OFF." << std::endl;
    }
}

AUDIO_SDK_API void AudioSDK_SetOcclusion(float amount)
{
    if (!gSource)
    {
        return;
    }

    if (amount < 0.0f)
    {
        amount = 0.0f;
    }

    if (amount > 1.0f)
    {
        amount = 1.0f;
    }

    gSource->SetOcclusion(amount);
}

AUDIO_SDK_API int AudioSDK_IsHRTFEnabled()
{
    if (!gEngine)
    {
        return 0;
    }

    return gEngine->IsHRTFEnabled() ? 1 : 0;
}