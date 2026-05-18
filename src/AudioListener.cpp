#include "AudioListener.h"

void AudioListener::SetPosition(const Vector3 &position)
{
    alListener3f(
        AL_POSITION,
        position.x,
        position.y,
        position.z);
}

void AudioListener::SetVelocity(const Vector3 &velocity)
{
    alListener3f(
        AL_VELOCITY,
        velocity.x,
        velocity.y,
        velocity.z);
}

void AudioListener::SetOrientation(const Vector3 &forward, const Vector3 &up)
{
    ALfloat orientation[] = {
        forward.x, forward.y, forward.z,
        up.x, up.y, up.z};

    alListenerfv(AL_ORIENTATION, orientation);
}

void AudioListener::SetPosition(float x, float y, float z)
{
    SetPosition(Vector3(x, y, z));
}

void AudioListener::SetVelocity(float x, float y, float z)
{
    SetVelocity(Vector3(x, y, z));
}