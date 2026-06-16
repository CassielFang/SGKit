#pragma once

#include <cstring>
#include <sgkit/math/MathUtils.h>
#include <sgkit/math/Vector3.h>
#include <sgkit/math/Vector4.h>

namespace sgkit {
namespace math {

// Column-major 4x4 matrix (matches OpenGL convention).
// Elements are indexed as m[col][row]:
//   m[0][0] m[1][0] m[2][0] m[3][0]    (column 0)
//   m[0][1] m[1][1] m[2][1] m[3][1]    (column 1)
//   m[0][2] m[1][2] m[2][2] m[3][2]    (column 2)
//   m[0][3] m[1][3] m[2][3] m[3][3]    (column 3)
struct Matrix4
{
    float m[4][4];

    Matrix4();

    // -- Raw data access (for passing to OpenGL) ----------------
    const float* Data() const { return &m[0][0]; }
    float* Data()             { return &m[0][0]; }

    // -- Element access ----------------------------------------
    float& operator()(int col, int row)       { return m[col][row]; }
    float  operator()(int col, int row) const { return m[col][row]; }

    // -- Comparison --------------------------------------------
    bool operator==(const Matrix4& rhs) const;
    bool operator!=(const Matrix4& rhs) const { return !(*this == rhs); }

    // -- Arithmetic --------------------------------------------
    Matrix4 operator*(const Matrix4& rhs) const;
    Vector4 operator*(const Vector4& v) const;
    Vector3 TransformPoint(const Vector3& v) const;
    Vector3 TransformDirection(const Vector3& v) const;

    // -- Initialization ----------------------------------------
    Matrix4& SetIdentity();
    Matrix4& SetZero();
    Matrix4& SetTranslate(float x, float y, float z);
    Matrix4& SetTranslate(const Vector3& v);
    Matrix4& SetRotateX(float radians);
    Matrix4& SetRotateY(float radians);
    Matrix4& SetRotateZ(float radians);
    Matrix4& SetScale(float x, float y, float z);
    Matrix4& SetScale(const Vector3& v);
    Matrix4& SetPerspective(float fovYRadians, float aspect, float nearZ, float farZ);
    Matrix4& SetOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ);
    Matrix4& SetLookAt(const Vector3& eye, const Vector3& target, const Vector3& up);

    // -- Operations --------------------------------------------
    Matrix4& Transpose();
    Matrix4 Transposed() const;

    Matrix4& Invert();
    Matrix4 Inverse() const;

    float Determinant() const;

    // -- Static factories --------------------------------------
    static Matrix4 Identity();
    static Matrix4 Zero();
    static Matrix4 Translate(const Vector3& v);
    static Matrix4 RotateX(float radians);
    static Matrix4 RotateY(float radians);
    static Matrix4 RotateZ(float radians);
    static Matrix4 Scale(const Vector3& v);
    static Matrix4 Perspective(float fovYRadians, float aspect, float nearZ, float farZ);
    static Matrix4 Orthographic(float left, float right, float bottom, float top, float nearZ, float farZ);
    static Matrix4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
};

} // namespace math
} // namespace sgkit
