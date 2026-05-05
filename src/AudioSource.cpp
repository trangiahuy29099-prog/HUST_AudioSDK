#include "../include/AudioSource.h"
#include <iostream>

#define DR_WAV_IMPLEMENTATION
#include "../include/dr_wav.h"

AudioSource::AudioSource() {
    alGenSources(1, &sourceId);
    alGenBuffers(1, &bufferId);
}

bool AudioSource::LoadWav(const std::string& filepath) {
    unsigned int channels, sampleRate;
    drwav_uint64 totalPCMFrameCount;
    
    drwav_int16* pSampleData = drwav_open_file_and_read_pcm_frames_s16(
        filepath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr) {
        std::cerr << "[AudioSDK] Lỗi file: " << filepath << std::endl;
        return false;
    }

    ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    ALsizei size = totalPCMFrameCount * channels * sizeof(drwav_int16);

    alBufferData(bufferId, format, pSampleData, size, sampleRate);
    alSourcei(sourceId, AL_BUFFER, bufferId);

    drwav_free(pSampleData, nullptr);
    return true;
}

void AudioSource::SetPosition(float x, float y, float z) { alSource3f(sourceId, AL_POSITION, x, y, z); }
void AudioSource::SetVelocity(float vx, float vy, float vz) { alSource3f(sourceId, AL_VELOCITY, vx, vy, vz); }
void AudioSource::Play() { alSourcePlay(sourceId); }
void AudioSource::SetLooping(bool loop) { alSourcei(sourceId, AL_LOOPING, loop ? AL_TRUE : AL_FALSE); }

AudioSource::~AudioSource() {
    alDeleteSources(1, &sourceId);
    alDeleteBuffers(1, &bufferId);
}
