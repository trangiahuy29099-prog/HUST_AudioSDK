#define AL_ALEXT_PROTOTYPES

#include "AudioSource.h"
#include "AudioEnvironment.h"

#include <AL/efx.h>

#include <algorithm>
#include <iostream>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

AudioSource::AudioSource()
    : sourceId(0),
      bufferId(0),
      occlusionFilter(0),
      occlusionFilterCreated(false)
{
    alGenSources(1, &sourceId);
    alGenBuffers(1, &bufferId);

    alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_FALSE);

    SetGain(1.0f);
    SetPitch(1.0f);

    SetReferenceDistance(5.0f);
    SetMaxDistance(100.0f);
    SetRolloffFactor(1.0f);

    if (alIsExtensionPresent("AL_EXT_EFX"))
    {
        alGenFilters(1, &occlusionFilter);

        if (alGetError() == AL_NO_ERROR && occlusionFilter != 0)
        {
            alFilteri(
                occlusionFilter,
                AL_FILTER_TYPE,
                AL_FILTER_LOWPASS);

            alFilterf(
                occlusionFilter,
                AL_LOWPASS_GAIN,
                1.0f);

            alFilterf(
                occlusionFilter,
                AL_LOWPASS_GAINHF,
                1.0f);

            if (alGetError() == AL_NO_ERROR)
            {
                occlusionFilterCreated = true;
            }
            else
            {
                alDeleteFilters(1, &occlusionFilter);

                occlusionFilter = 0;
                occlusionFilterCreated = false;
            }
        }
    }
}

AudioSource::~AudioSource()
{
    Stop();

    DetachEnvironment();

    if (occlusionFilterCreated && occlusionFilter != 0)
    {
        alSourcei(
            sourceId,
            AL_DIRECT_FILTER,
            AL_FILTER_NULL);

        alDeleteFilters(1, &occlusionFilter);

        occlusionFilter = 0;
        occlusionFilterCreated = false;
    }

    if (sourceId != 0)
    {
        alDeleteSources(1, &sourceId);
        sourceId = 0;
    }

    if (bufferId != 0)
    {
        alDeleteBuffers(1, &bufferId);
        bufferId = 0;
    }
}

bool AudioSource::LoadWav(const std::string &filepath)
{
    unsigned int channels = 0;
    unsigned int sampleRate = 0;
    drwav_uint64 totalPCMFrameCount = 0;

    drwav_int16 *pSampleData = drwav_open_file_and_read_pcm_frames_s16(
        filepath.c_str(),
        &channels,
        &sampleRate,
        &totalPCMFrameCount,
        nullptr);

    if (pSampleData == nullptr)
    {
        std::cerr << "[AudioSource] Failed to load WAV file: "
                  << filepath
                  << std::endl;

        return false;
    }

    ALenum format = 0;

    if (channels == 1)
    {
        format = AL_FORMAT_MONO16;
    }
    else if (channels == 2)
    {
        format = AL_FORMAT_STEREO16;

        std::cout << "[AudioSource] Warning: stereo WAV loaded. "
                  << "For clear 3D spatial audio and HRTF, use a mono WAV file."
                  << std::endl;
    }
    else
    {
        std::cerr << "[AudioSource] Unsupported channel count: "
                  << channels
                  << std::endl;

        drwav_free(pSampleData, nullptr);

        return false;
    }

    ALsizei size = static_cast<ALsizei>(
        totalPCMFrameCount * channels * sizeof(drwav_int16));

    alBufferData(
        bufferId,
        format,
        pSampleData,
        size,
        static_cast<ALsizei>(sampleRate));

    drwav_free(pSampleData, nullptr);

    if (alGetError() != AL_NO_ERROR)
    {
        std::cerr << "[AudioSource] Failed to upload WAV data to OpenAL buffer." << std::endl;
        return false;
    }

    alSourcei(
        sourceId,
        AL_BUFFER,
        static_cast<ALint>(bufferId));

    if (alGetError() != AL_NO_ERROR)
    {
        std::cerr << "[AudioSource] Failed to attach buffer to source." << std::endl;
        return false;
    }

    std::cout << "[AudioSource] Loaded WAV: "
              << filepath
              << " | channels: "
              << channels
              << " | sample rate: "
              << sampleRate
              << std::endl;

    return true;
}

void AudioSource::Play()
{
    alSourcePlay(sourceId);
}

void AudioSource::Pause()
{
    alSourcePause(sourceId);
}

void AudioSource::Stop()
{
    alSourceStop(sourceId);
}

bool AudioSource::IsPlaying() const
{
    ALint state = AL_STOPPED;

    alGetSourcei(
        sourceId,
        AL_SOURCE_STATE,
        &state);

    return state == AL_PLAYING;
}

void AudioSource::SetLooping(bool loop)
{
    alSourcei(
        sourceId,
        AL_LOOPING,
        loop ? AL_TRUE : AL_FALSE);
}

void AudioSource::SetPosition(const Vector3 &position)
{
    alSource3f(
        sourceId,
        AL_POSITION,
        position.x,
        position.y,
        position.z);
}

void AudioSource::SetVelocity(const Vector3 &velocity)
{
    alSource3f(
        sourceId,
        AL_VELOCITY,
        velocity.x,
        velocity.y,
        velocity.z);
}

void AudioSource::SetPosition(float x, float y, float z)
{
    SetPosition(Vector3(x, y, z));
}

void AudioSource::SetVelocity(float x, float y, float z)
{
    SetVelocity(Vector3(x, y, z));
}

void AudioSource::SetGain(float gain)
{
    alSourcef(
        sourceId,
        AL_GAIN,
        gain);
}

void AudioSource::SetPitch(float pitch)
{
    alSourcef(
        sourceId,
        AL_PITCH,
        pitch);
}

void AudioSource::SetReferenceDistance(float distance)
{
    alSourcef(
        sourceId,
        AL_REFERENCE_DISTANCE,
        distance);
}

void AudioSource::SetMaxDistance(float distance)
{
    alSourcef(
        sourceId,
        AL_MAX_DISTANCE,
        distance);
}

void AudioSource::SetRolloffFactor(float factor)
{
    alSourcef(
        sourceId,
        AL_ROLLOFF_FACTOR,
        factor);
}

void AudioSource::AttachEnvironment(const AudioEnvironment &environment)
{
    if (!environment.IsReady())
    {
        std::cerr << "[AudioSource] Cannot attach environment because it is not ready." << std::endl;
        return;
    }

    alSource3i(
        sourceId,
        AL_AUXILIARY_SEND_FILTER,
        static_cast<ALint>(environment.GetSlot()),
        0,
        AL_FILTER_NULL);
}

void AudioSource::DetachEnvironment()
{
    alSource3i(
        sourceId,
        AL_AUXILIARY_SEND_FILTER,
        AL_EFFECTSLOT_NULL,
        0,
        AL_FILTER_NULL);
}

void AudioSource::SetOcclusion(float amount)
{
    amount = std::clamp(amount, 0.0f, 1.0f);

    float directGain = 1.0f - amount * 0.45f;

    SetGain(directGain);

    if (!occlusionFilterCreated || occlusionFilter == 0)
    {
        return;
    }

    float highFrequencyGain = 1.0f - amount * 0.85f;

    if (highFrequencyGain < 0.05f)
    {
        highFrequencyGain = 0.05f;
    }

    alFilteri(
        occlusionFilter,
        AL_FILTER_TYPE,
        AL_FILTER_LOWPASS);

    alFilterf(
        occlusionFilter,
        AL_LOWPASS_GAIN,
        directGain);

    alFilterf(
        occlusionFilter,
        AL_LOWPASS_GAINHF,
        highFrequencyGain);

    alSourcei(
        sourceId,
        AL_DIRECT_FILTER,
        static_cast<ALint>(occlusionFilter));
}

ALuint AudioSource::GetSourceId() const
{
    return sourceId;
}