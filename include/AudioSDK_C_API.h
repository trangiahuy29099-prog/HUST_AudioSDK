#pragma once

#ifdef _WIN32
#define AUDIO_SDK_API extern "C" __declspec(dllexport)
#else
#define AUDIO_SDK_API extern "C"
#endif

AUDIO_SDK_API int AudioSDK_Init();
AUDIO_SDK_API void AudioSDK_Shutdown();

AUDIO_SDK_API int AudioSDK_LoadSound(const char *filepath);
AUDIO_SDK_API int AudioSDK_LoadSoundFromMemory(const unsigned char *data, int dataSize);
AUDIO_SDK_API void AudioSDK_Play();
AUDIO_SDK_API void AudioSDK_Stop();
AUDIO_SDK_API void AudioSDK_SetLooping(int loop);

AUDIO_SDK_API void AudioSDK_SetSourcePosition(float x, float y, float z);
AUDIO_SDK_API void AudioSDK_SetSourceVelocity(float x, float y, float z);

AUDIO_SDK_API void AudioSDK_SetListenerPosition(float x, float y, float z);
AUDIO_SDK_API void AudioSDK_SetListenerVelocity(float x, float y, float z);

AUDIO_SDK_API void AudioSDK_SetListenerOrientation(
    float fx, float fy, float fz,
    float ux, float uy, float uz);

AUDIO_SDK_API void AudioSDK_SetReverb(int enabled);
AUDIO_SDK_API void AudioSDK_SetOcclusion(float amount);

AUDIO_SDK_API int AudioSDK_IsHRTFEnabled();