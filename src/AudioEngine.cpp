#include "../include/AudioEngine.h"

AudioEngine::AudioEngine() : device(nullptr), context(nullptr) {}

void AudioEngine::Init() {
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "[AudioSDK] Lỗi: Không thể mở thiết bị âm thanh!" << std::endl;
        return;
    }
    context = alcCreateContext(device, nullptr);
    alcMakeContextCurrent(context);
    std::cout << "[AudioSDK] Khởi tạo OpenAL trên Ubuntu thành công." << std::endl;
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
}

void AudioEngine::SetDopplerFactor(float factor, float velocityOfSound) {
    alDopplerFactor(factor);
    alSpeedOfSound(velocityOfSound);
    std::cout << "[AudioSDK] Đã thiết lập môi trường Doppler." << std::endl;
}

AudioEngine::~AudioEngine() {
    alcMakeContextCurrent(nullptr);
    if (context) alcDestroyContext(context);
    if (device) alcCloseDevice(device);
}
