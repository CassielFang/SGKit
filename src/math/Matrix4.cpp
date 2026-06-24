#include <sgkit/math/Matrix4.h>

#include <cmath>
#include <cstring>

namespace sgkit {
namespace math {

// -- Construct / Set -----------------------------------------------

Matrix4::Matrix4()
{
    SetIdentity();
}

Matrix4& Matrix4::SetIdentity()
{
    std::memset(m, 0, sizeof(m));
    m[0][0] = 1.0f;
    m[1][1] = 1.0f;
    m[2][2] = 1.0f;
    m[3][3] = 1.0f;
    return *this;
}

Matrix4& Matrix4::SetZero()
{
    std::memset(m, 0, sizeof(m));
    return *this;
}

Matrix4& Matrix4::SetTranslate(float x, float y, float z)
{
    SetIdentity();
    m[3][0] = x;
    m[3][1] = y;
    m[3][2] = z;
    return *this;
}

Matrix4& Matrix4::SetTranslate(const Vector3& v)
{
    return SetTranslate(v.x, v.y, v.z);
}

Matrix4& Matrix4::SetRotateX(float radians)
{
    float c = std::cos(radians);
    float s = std::sin(radians);
    SetIdentity();
    m[1][1] = c;
    m[1][2] = s;
    m[2][1] = -s;
    m[2][2] = c;
    return *this;
}

Matrix4& Matrix4::SetRotateY(float radians)
{
    float c = std::cos(radians);
    float s = std::sin(radians);
    SetIdentity();
    m[0][0] = c;
    m[0][2] = -s;
    m[2][0] = s;
    m[2][2] = c;
    return *this;
}

Matrix4& Matrix4::SetRotateZ(float radians)
{
    float c = std::cos(radians);
    float s = std::sin(radians);
    SetIdentity();
    m[0][0] = c;
    m[0][1] = s;
    m[1][0] = -s;
    m[1][1] = c;
    return *this;
}

Matrix4& Matrix4::SetScale(float x, float y, float z)
{
    SetIdentity();
    m[0][0] = x;
    m[1][1] = y;
    m[2][2] = z;
    return *this;
}

Matrix4& Matrix4::SetScale(const Vector3& v)
{
    return SetScale(v.x, v.y, v.z);
}

Matrix4& Matrix4::SetPerspective(float fovYRadians, float aspect, float nearZ, float farZ)
{
    float tanHalfFov = std::tan(fovYRadians * 0.5f);
    SetZero();
    m[0][0] = 1.0f / (aspect * tanHalfFov);
    m[1][1] = 1.0f / tanHalfFov;
    m[2][2] = -(farZ + nearZ) / (farZ - nearZ);
    m[2][3] = -1.0f;
    m[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    return *this;
}

Matrix4& Matrix4::SetOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ)
{
    SetZero();
    m[0][0] = 2.0f / (right - left);
    m[1][1] = 2.0f / (top - bottom);
    m[2][2] = -2.0f / (farZ - nearZ);
    m[3][0] = -(right + left) / (right - left);
    m[3][1] = -(top + bottom) / (top - bottom);
    m[3][2] = -(farZ + nearZ) / (farZ - nearZ);
    m[3][3] = 1.0f;
    return *this;
}

Matrix4& Matrix4::SetLookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    Vector3 f = (target - eye).Normalized();
    Vector3 s = Vector3::Cross(f, up).Normalized();
    Vector3 u = Vector3::Cross(s, f);

    SetIdentity();
    m[0][0] =  s.x;
    m[1][0] =  s.y;
    m[2][0] =  s.z;
    m[0][1] =  u.x;
    m[1][1] =  u.y;
    m[2][1] =  u.z;
    m[0][2] = -f.x;
    m[1][2] = -f.y;
    m[2][2] = -f.z;
    m[3][0] = -Vector3::Dot(s, eye);
    m[3][1] = -Vector3::Dot(u, eye);
    m[3][2] =  Vector3::Dot(f, eye);
    return *this;
}

// -- Comparison ----------------------------------------------------

bool Matrix4::operator==(const Matrix4& rhs) const
{
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (!Approximately(m[i][j], rhs.m[i][j]))
                return false;
    return true;
}

// -- Matrix multiplication -----------------------------------------

Matrix4 Matrix4::operator*(const Matrix4& rhs) const
{
    Matrix4 result;
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            result.m[col][row] =
                m[0][row] * rhs.m[col][0] +
                m[1][row] * rhs.m[col][1] +
                m[2][row] * rhs.m[col][2] +
                m[3][row] * rhs.m[col][3];
        }
    }
    return result;
}

Vector4 Matrix4::operator*(const Vector4& v) const
{
    return {
        m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w,
        m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w,
        m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w,
        m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w
    };
}

Vector3 Matrix4::TransformPoint(const Vector3& v) const
{
    Vector4 r = (*this) * Vector4(v, 1.0f);
    if (std::fabs(r.w) > k_Epsilon)
        return {r.x / r.w, r.y / r.w, r.z / r.w};
    return {r.x, r.y, r.z};
}

Vector3 Matrix4::TransformDirection(const Vector3& v) const
{
    Vector4 r = (*this) * Vector4(v, 0.0f);
    return {r.x, r.y, r.z};
}

// -- Transpose -----------------------------------------------------

Matrix4& Matrix4::Transpose()
{
    for (int i = 0; i < 4; ++i)
    {
        for (int j = i + 1; j < 4; ++j)
        {
            float tmp = m[i][j];
            m[i][j] = m[j][i];
            m[j][i] = tmp;
        }
    }
    return *this;
}

Matrix4 Matrix4::Transposed() const
{
    Matrix4 result = *this;
    result.Transpose();
    return result;
}

// -- Determinant ---------------------------------------------------

float Matrix4::Determinant() const
{
    float a0 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    float a1 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
    float a2 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
    float a3 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    float a4 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
    float a5 = m[0][2] * m[1][3] - m[0][3] * m[1][2];
    float b0 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
    float b1 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
    float b2 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
    float b3 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
    float b4 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
    float b5 = m[2][2] * m[3][3] - m[2][3] * m[3][2];

    return a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
}

// -- Inverted -------------------------------------------------------

Matrix4& Matrix4::Invert()
{
    *this = Inverted();
    return *this;
}

Matrix4 Matrix4::Inverted() const
{
    float a0 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    float a1 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
    float a2 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
    float a3 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    float a4 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
    float a5 = m[0][2] * m[1][3] - m[0][3] * m[1][2];
    float b0 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
    float b1 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
    float b2 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
    float b3 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
    float b4 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
    float b5 = m[2][2] * m[3][3] - m[2][3] * m[3][2];

    float det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
    if (std::fabs(det) < k_Epsilon)
    {
        return Identity();  // degenerated matrix -> return identity
    }

    Matrix4 result;
    float invDet = 1.0f / det;

    result.m[0][0] = (+m[1][1] * b5 - m[1][2] * b4 + m[1][3] * b3) * invDet;
    result.m[0][1] = (-m[0][1] * b5 + m[0][2] * b4 - m[0][3] * b3) * invDet;
    result.m[0][2] = (+m[3][1] * a5 - m[3][2] * a4 + m[3][3] * a3) * invDet;
    result.m[0][3] = (-m[2][1] * a5 + m[2][2] * a4 - m[2][3] * a3) * invDet;

    result.m[1][0] = (-m[1][0] * b5 + m[1][2] * b2 - m[1][3] * b1) * invDet;
    result.m[1][1] = (+m[0][0] * b5 - m[0][2] * b2 + m[0][3] * b1) * invDet;
    result.m[1][2] = (-m[3][0] * a5 + m[3][2] * a2 - m[3][3] * a1) * invDet;
    result.m[1][3] = (+m[2][0] * a5 - m[2][2] * a2 + m[2][3] * a1) * invDet;

    result.m[2][0] = (+m[1][0] * b4 - m[1][1] * b2 + m[1][3] * b0) * invDet;
    result.m[2][1] = (-m[0][0] * b4 + m[0][1] * b2 - m[0][3] * b0) * invDet;
    result.m[2][2] = (+m[3][0] * a4 - m[3][1] * a2 + m[3][3] * a0) * invDet;
    result.m[2][3] = (-m[2][0] * a4 + m[2][1] * a2 - m[2][3] * a0) * invDet;

    result.m[3][0] = (-m[1][0] * b3 + m[1][1] * b1 - m[1][2] * b0) * invDet;
    result.m[3][1] = (+m[0][0] * b3 - m[0][1] * b1 + m[0][2] * b0) * invDet;
    result.m[3][2] = (-m[3][0] * a3 + m[3][1] * a1 - m[3][2] * a0) * invDet;
    result.m[3][3] = (+m[2][0] * a3 - m[2][1] * a1 + m[2][2] * a0) * invDet;

    return result;
}

// -- Static factories ----------------------------------------------

Matrix4 Matrix4::Identity() { Matrix4 m; m.SetIdentity(); return m; }
Matrix4 Matrix4::Zero()     { Matrix4 m; m.SetZero();     return m; }
Matrix4 Matrix4::Translate(const Vector3& v) { Matrix4 m; m.SetTranslate(v); return m; }
Matrix4 Matrix4::RotateX(float r)            { Matrix4 m; m.SetRotateX(r);   return m; }
Matrix4 Matrix4::RotateY(float r)            { Matrix4 m; m.SetRotateY(r);   return m; }
Matrix4 Matrix4::RotateZ(float r)            { Matrix4 m; m.SetRotateZ(r);   return m; }
Matrix4 Matrix4::Scale(const Vector3& v)     { Matrix4 m; m.SetScale(v);     return m; }

Matrix4 Matrix4::Perspective(float fovYRadians, float aspect, float nearZ, float farZ)
{
    Matrix4 m; m.SetPerspective(fovYRadians, aspect, nearZ, farZ); return m;
}

Matrix4 Matrix4::Orthographic(float l, float r, float b, float t, float n, float f)
{
    Matrix4 m; m.SetOrthographic(l, r, b, t, n, f); return m;
}

Matrix4 Matrix4::LookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    Matrix4 m; m.SetLookAt(eye, target, up); return m;
}

} // namespace math
} // namespace sgkit
