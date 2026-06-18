#pragma once

#include <sgkit/math/MathUtils.h>
#include <sgkit/math/Vector2.h>
#include <sgkit/math/Vector3.h>

namespace sgkit {
namespace math {

class Vector4
{
public:
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    Vector4() = default;
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    explicit Vector4(const Vector3& v3, float w = 1.0f) : x(v3.x), y(v3.y), z(v3.z), w(w) {}
    explicit Vector4(const Vector2& v2, float z = 0.0f, float w = 1.0f) : x(v2.x), y(v2.y), z(z), w(w) {}

    // -- Swizzle helpers ---------------------------------------
    Vector3 XYZ() const { return {x, y, z}; }
    Vector2 XY()  const { return {x, y}; }

    // -- Element access ----------------------------------------
    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    // -- Arithmetic --------------------------------------------
    Vector4 operator+(const Vector4& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w}; }
    Vector4 operator-(const Vector4& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w}; }
    Vector4 operator*(float s) const           { return {x * s, y * s, z * s, w * s}; }
    Vector4 operator/(float s) const           { return {x / s, y / s, z / s, w / s}; }

    Vector4& operator+=(const Vector4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
    Vector4& operator-=(const Vector4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
    Vector4& operator*=(float s)            { x *= s; y *= s; z *= s; w *= s; return *this; }
    Vector4& operator/=(float s)            { x /= s; y /= s; z /= s; w /= s; return *this; }

    Vector4 operator-() const { return {-x, -y, -z, -w}; }

    bool operator==(const Vector4& rhs) const
    {
        return Approximately(x, rhs.x) && Approximately(y, rhs.y) &&
               Approximately(z, rhs.z) && Approximately(w, rhs.w);
    }
    bool operator!=(const Vector4& rhs) const { return !(*this == rhs); }

    // -- Vector operations -------------------------------------
    float LengthSquared() const { return x * x + y * y + z * z + w * w; }
    float Length() const        { return std::sqrt(LengthSquared()); }

    Vector4& Normalize()
    {
        float len = Length();
        if (len > k_Epsilon) { x /= len; y /= len; z /= len; w /= len; }
        return *this;
    }

    Vector4 Normalized() const { Vector4 v = *this; v.Normalize(); return v; }

    static float Dot(const Vector4& a, const Vector4& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    static Vector4 Lerp(const Vector4& a, const Vector4& b, float t)
    {
        return {
            sgkit::math::Lerp(a.x, b.x, t),
            sgkit::math::Lerp(a.y, b.y, t),
            sgkit::math::Lerp(a.z, b.z, t),
            sgkit::math::Lerp(a.w, b.w, t)
        };
    }

    static const Vector4 k_Zero;
    static const Vector4 k_One;
};

inline Vector4 operator*(float s, const Vector4& v) { return v * s; }

} // namespace math
} // namespace sgkit
