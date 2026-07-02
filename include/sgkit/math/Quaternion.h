#pragma once

#include <sgkit/math/Vector3.h>

namespace sgkit {
namespace math {

class Quaternion
{
public:
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f; // identity quaternion

    Quaternion() = default;
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    // -- Comparison
    bool operator==(const Quaternion& rhs) const;
    bool operator!=(const Quaternion& rhs) const { return !(*this == rhs); }

    // -- Operations
    Quaternion operator*(const Quaternion& rhs) const;
    Vector3 operator*(const Vector3& v) const; // rotate vector

    Quaternion& Normalize();
    Quaternion Normalized() const;

    Quaternion& Conjugate();
    Quaternion Conjugated() const;

    Quaternion& Invert();
    Quaternion Inverted() const;

    float LengthSquared() const;
    float Length() const;
    float Dot(const Quaternion& rhs) const;

    // -- Conversion
    Vector3 ToEulerAngles() const; // returns {pitch, yaw, roll} in radians

    // -- Static factories
    static Quaternion Identity();
    static Quaternion FromEulerAngles(float pitch, float yaw, float roll);
    static Quaternion FromAxisAngle(const Vector3& axis, float radians);
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t);
    static Quaternion LookAt(const Vector3& direction, const Vector3& up);
};

}
}
