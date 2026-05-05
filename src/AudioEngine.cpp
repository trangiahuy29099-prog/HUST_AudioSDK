#include "../include/AudioEngine.h"
#include <AL/alext.h> // Thư viện chứa các extension như HRTF

AudioEngine::AudioEngine() : device(nullptr), context(nullptr) {}

void AudioEngine::Init() {
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "[AudioSDK] Lỗi: Không thể mở thiết bị!" << std::endl;
        return;
    }

    // Yêu cầu OpenAL Soft kích hoạt bộ lọc HRTF
    ALCint attrs[] = { ALC_HRTF_SOFT, ALC_TRUE, 0 };
    context = alcCreateContext(device, attrs);
    alcMakeContextCurrent(context);

    // Kiểm tra xem hệ thống đã bật HRTF thành công chưa
    ALCint hrtf_state;
    alcGetIntegerv(device, ALC_HRTF_SOFT, 1, &hrtf_state);
    if (hrtf_state) {
        std::cout << ">>> [AUDIO ENGINE] HRTF KICH HOAT: San sang cho khong gian 360 do! <<<" << std::endl;
    } else {
        std::cout << "[AUDIO ENGINE] Canh bao: He thong khong the bat HRTF." << std::endl;
    }

    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
}

void AudioEngine::SetDopplerFactor(float factor, float velocityOfSound) {
    alDopplerFactor(factor);
    alSpeedOfSound(velocityOfSound);
}

AudioEngine::~AudioEngine() {
    alcMakeContextCurrent(nullptr);
    if (context) alcDestroyContext(context);
    if (device) alcCloseDevice(device);
}
