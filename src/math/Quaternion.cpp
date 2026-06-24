#include <sgkit/math/Quaternion.h>

#include <cmath>

namespace sgkit {
namespace math {

// -- Comparison ----------------------------------------------------

bool Quaternion::operator==(const Quaternion& rhs) const
{
    return Approximately(x, rhs.x) && Approximately(y, rhs.y) &&
           Approximately(z, rhs.z) && Approximately(w, rhs.w);
}

// -- Multiplication ------------------------------------------------

Quaternion Quaternion::operator*(const Quaternion& rhs) const
{
    return {
        w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
        w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
        w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
        w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
    };
}

Vector3 Quaternion::operator*(const Vector3& v) const
{
    // q * v * q^-1 (optimised)
    Vector3 qv{x, y, z};
    Vector3 uv = Vector3::Cross(qv, v);
    Vector3 uuv = Vector3::Cross(qv, uv);
    return v + (uv * w + uuv) * 2.0f;
}

// -- Normalise / Conjugate / Inverse -------------------------------

Quaternion& Quaternion::Normalize()
{
    float len = Length();
    if (len > k_Epsilon) { x /= len; y /= len; z /= len; w /= len; }
    return *this;
}

Quaternion Quaternion::Normalized() const { Quaternion q = *this; q.Normalize(); return q; }

Quaternion& Quaternion::Conjugate() { x = -x; y = -y; z = -z; return *this; }
Quaternion Quaternion::Conjugated() const { return {-x, -y, -z, w}; }

Quaternion& Quaternion::Invert() { *this = Inverted(); return *this; }
Quaternion Quaternion::Inverted() const
{
    float lenSq = LengthSquared();
    if (lenSq < k_Epsilon) return *this;
    Quaternion c = Conjugated();
    float inv = 1.0f / lenSq;
    return {c.x * inv, c.y * inv, c.z * inv, c.w * inv};
}

float Quaternion::LengthSquared() const { return x * x + y * y + z * z + w * w; }
float Quaternion::Length() const { return std::sqrt(LengthSquared()); }
float Quaternion::Dot(const Quaternion& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }

// -- Euler conversion ----------------------------------------------

Vector3 Quaternion::ToEulerAngles() const
{
    // pitch (x), yaw (y), roll (z)
    float sinPitch = 2.0f * (w * x + y * z);
    float cosPitch = 1.0f - 2.0f * (x * x + y * y);
    float pitch = std::atan2(sinPitch, cosPitch);

    float sinYaw = 2.0f * (w * y - z * x);
    float yaw;
    if (std::fabs(sinYaw) >= 1.0f)
        yaw = std::copysign(k_HalfPi, sinYaw);
    else
        yaw = std::asin(sinYaw);

    float sinRoll = 2.0f * (w * z + x * y);
    float cosRoll = 1.0f - 2.0f * (y * y + z * z);
    float roll = std::atan2(sinRoll, cosRoll);

    return {pitch, yaw, roll};
}

// -- Static factories ----------------------------------------------

Quaternion Quaternion::Identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }

Quaternion Quaternion::FromEulerAngles(float pitch, float yaw, float roll)
{
    float hp = pitch * 0.5f, hy = yaw * 0.5f, hr = roll * 0.5f;
    float cp = std::cos(hp), sp = std::sin(hp);
    float cy = std::cos(hy), sy = std::sin(hy);
    float cr = std::cos(hr), sr = std::sin(hr);

    return {
        sp * cy * cr - cp * sy * sr,
        cp * sy * cr + sp * cy * sr,
        cp * cy * sr - sp * sy * cr,
        cp * cy * cr + sp * sy * sr
    };
}

Quaternion Quaternion::FromAxisAngle(const Vector3& axis, float radians)
{
    float halfAngle = radians * 0.5f;
    float s = std::sin(halfAngle);
    return {axis.x * s, axis.y * s, axis.z * s, std::cos(halfAngle)};
}

Quaternion Quaternion::Slerp(const Quaternion& a, const Quaternion& b, float t)
{
    float cosTheta = a.Dot(b);
    Quaternion bAdj = b;
    if (cosTheta < 0.0f) { cosTheta = -cosTheta; bAdj = {-b.x, -b.y, -b.z, -b.w}; }

    if (cosTheta > 1.0f - k_Epsilon)
    {
        // Linear interpolation for small angles
        return {
            sgkit::math::Lerp(a.x, bAdj.x, t),
            sgkit::math::Lerp(a.y, bAdj.y, t),
            sgkit::math::Lerp(a.z, bAdj.z, t),
            sgkit::math::Lerp(a.w, bAdj.w, t)
        };
    }

    float theta = std::acos(cosTheta);
    float sinTheta = std::sin(theta);
    float wA = std::sin((1.0f - t) * theta) / sinTheta;
    float wB = std::sin(t * theta) / sinTheta;
    return {
        a.x * wA + bAdj.x * wB,
        a.y * wA + bAdj.y * wB,
        a.z * wA + bAdj.z * wB,
        a.w * wA + bAdj.w * wB
    };
}

Quaternion Quaternion::LookAt(const Vector3& direction, const Vector3& up)
{
    Vector3 f = direction.Normalized();
    Vector3 s = Vector3::Cross(up, f).Normalized();
    Vector3 u = Vector3::Cross(f, s);

    // Rotation matrix -> quaternion
    float trace = s.x + u.y + f.z;
    if (trace > 0.0f)
    {
        float r = std::sqrt(1.0f + trace);
        float inv = 0.5f / r;
        return {(u.z - f.y) * inv, (f.x - s.z) * inv, (s.y - u.x) * inv, r * 0.5f};
    }
    else if (s.x > u.y && s.x > f.z)
    {
        float r = std::sqrt(1.0f + s.x - u.y - f.z);
        float inv = 0.5f / r;
        return {r * 0.5f, (u.x + s.y) * inv, (f.x + s.z) * inv, (u.z - f.y) * inv};
    }
    else if (u.y > f.z)
    {
        float r = std::sqrt(1.0f + u.y - s.x - f.z);
        float inv = 0.5f / r;
        return {(u.x + s.y) * inv, r * 0.5f, (f.y + u.z) * inv, (f.x - s.z) * inv};
    }
    else
    {
        float r = std::sqrt(1.0f + f.z - s.x - u.y);
        float inv = 0.5f / r;
        return {(f.x + s.z) * inv, (f.y + u.z) * inv, r * 0.5f, (s.y - u.x) * inv};
    }
}

} // namespace math
} // namespace sgkit
