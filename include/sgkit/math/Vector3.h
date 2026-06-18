#pragma once

#include <sgkit/math/MathUtils.h>
#include <sgkit/math/Vector2.h>

namespace sgkit {
namespace math {

class Vector3
{
public:
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3() = default;
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    explicit Vector3(const Vector2& v2, float z = 0.0f) : x(v2.x), y(v2.y), z(z) {}

    // -- Swizzle helpers ---------------------------------------
    Vector2 XY() const { return {x, y}; }
    Vector2 XZ() const { return {x, z}; }
    Vector2 YZ() const { return {y, z}; }

    // -- Element access ----------------------------------------
    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    // -- Arithmetic --------------------------------------------
    Vector3 operator+(const Vector3& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    Vector3 operator-(const Vector3& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    Vector3 operator*(float s) const           { return {x * s, y * s, z * s}; }
    Vector3 operator/(float s) const           { return {x / s, y / s, z / s}; }

    Vector3& operator+=(const Vector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
    Vector3& operator-=(const Vector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
    Vector3& operator*=(float s)            { x *= s; y *= s; z *= s; return *this; }
    Vector3& operator/=(float s)            { x /= s; y /= s; z /= s; return *this; }

    Vector3 operator-() const { return {-x, -y, -z}; }

    bool operator==(const Vector3& rhs) const
    {
        return Approximately(x, rhs.x) && Approximately(y, rhs.y) && Approximately(z, rhs.z);
    }
    bool operator!=(const Vector3& rhs) const { return !(*this == rhs); }

    // -- Vector operations -------------------------------------
    float LengthSquared() const { return x * x + y * y + z * z; }
    float Length() const        { return std::sqrt(LengthSquared()); }

    Vector3& Normalize()
    {
        float len = Length();
        if (len > k_Epsilon) { x /= len; y /= len; z /= len; }
        return *this;
    }

    Vector3 Normalized() const { Vector3 v = *this; v.Normalize(); return v; }

    static float Dot(const Vector3& a, const Vector3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vector3 Cross(const Vector3& a, const Vector3& b)
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
    {
        return {
            sgkit::math::Lerp(a.x, b.x, t),
            sgkit::math::Lerp(a.y, b.y, t),
            sgkit::math::Lerp(a.z, b.z, t)
        };
    }

    static const Vector3 k_Zero;
    static const Vector3 k_One;
    static const Vector3 k_Up;
    static const Vector3 k_Down;
    static const Vector3 k_Right;
    static const Vector3 k_Left;
    static const Vector3 k_Forward;
    static const Vector3 k_Back;
};

inline Vector3 operator*(float s, const Vector3& v) { return v * s; }

} // namespace math
} // namespace sgkit
