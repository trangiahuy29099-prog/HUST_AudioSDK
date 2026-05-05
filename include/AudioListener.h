#pragma once
#include <AL/al.h>

class AudioListener {
public:
    static void SetPosition(float x, float y, float z) { alListener3f(AL_POSITION, x, y, z); }
    static void SetVelocity(float vx, float vy, float vz) { alListener3f(AL_VELOCITY, vx, vy, vz); }
};
