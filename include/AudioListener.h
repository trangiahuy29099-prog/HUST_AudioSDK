#pragma once

#include <AL/al.h>

#include "Vector3.h"

class AudioListener
{
public:
    static void SetPosition(const Vector3 &position);
    static void SetVelocity(const Vector3 &velocity);
    static void SetOrientation(const Vector3 &forward, const Vector3 &up);

    static void SetPosition(float x, float y, float z);
    static void SetVelocity(float x, float y, float z);
};