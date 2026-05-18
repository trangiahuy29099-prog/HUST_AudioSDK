#pragma once

#include <cmath>

struct Vector3
{
    float x;
    float y;
    float z;

    Vector3()
        : x(0.0f), y(0.0f), z(0.0f) {}

    Vector3(float x, float y, float z)
        : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3 &other) const
    {
        return Vector3(
            x + other.x,
            y + other.y,
            z + other.z);
    }

    Vector3 operator-(const Vector3 &other) const
    {
        return Vector3(
            x - other.x,
            y - other.y,
            z - other.z);
    }

    Vector3 operator*(float scalar) const
    {
        return Vector3(
            x * scalar,
            y * scalar,
            z * scalar);
    }

    Vector3 operator/(float scalar) const
    {
        if (scalar == 0.0f)
        {
            return Vector3();
        }

        return Vector3(
            x / scalar,
            y / scalar,
            z / scalar);
    }

    float Length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3 Normalized() const
    {
        float len = Length();

        if (len == 0.0f)
        {
            return Vector3();
        }

        return *this / len;
    }

    static float Distance(const Vector3 &a, const Vector3 &b)
    {
        return (a - b).Length();
    }
};