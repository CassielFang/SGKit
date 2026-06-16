#pragma once

#include <cmath>
#include <sgkit/math/MathUtils.h>

namespace sgkit {
namespace math {

struct Vector2
{
    float x = 0.0f;
    float y = 0.0f;

    Vector2() = default;
    Vector2(float x, float y) : x(x), y(y) {}

    // -- Element access ----------------------------------------
    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    // -- Arithmetic operators ----------------------------------
    Vector2 operator+(const Vector2& rhs) const { return {x + rhs.x, y + rhs.y}; }
    Vector2 operator-(const Vector2& rhs) const { return {x - rhs.x, y - rhs.y}; }
    Vector2 operator*(float s) const           { return {x * s, y * s}; }
    Vector2 operator/(float s) const           { return {x / s, y / s}; }

    Vector2& operator+=(const Vector2& rhs) { x += rhs.x; y += rhs.y; return *this; }
    Vector2& operator-=(const Vector2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    Vector2& operator*=(float s)            { x *= s; y *= s; return *this; }
    Vector2& operator/=(float s)            { x /= s; y /= s; return *this; }

    Vector2 operator-() const { return {-x, -y}; }

    bool operator==(const Vector2& rhs) const { return Approximately(x, rhs.x) && Approximately(y, rhs.y); }
    bool operator!=(const Vector2& rhs) const { return !(*this == rhs); }

    // -- Vector operations -------------------------------------
    float LengthSquared() const { return x * x + y * y; }
    float Length() const        { return std::sqrt(LengthSquared()); }

    Vector2& Normalize()
    {
        float len = Length();
        if (len > k_Epsilon) { x /= len; y /= len; }
        return *this;
    }

    Vector2 Normalized() const { Vector2 v = *this; v.Normalize(); return v; }

    static float Dot(const Vector2& a, const Vector2& b) { return a.x * b.x + a.y * b.y; }

    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t)
    {
        return {sgkit::math::Lerp(a.x, b.x, t), sgkit::math::Lerp(a.y, b.y, t)};
    }

    static const Vector2 k_Zero;
    static const Vector2 k_One;
    static const Vector2 k_Up;
    static const Vector2 k_Right;
};

inline Vector2 operator*(float s, const Vector2& v) { return v * s; }

} // namespace math
} // namespace sgkit
