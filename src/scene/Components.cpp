#include <sgkit/scene/Components.h>

namespace sgkit {
namespace scene {

namespace component {

math::Matrix4 Transform::GetLocalMatrix() const
{
    math::Matrix4 t = math::Matrix4::Translate(position);

    float xx = rotation.x * rotation.x;
    float yy = rotation.y * rotation.y;
    float zz = rotation.z * rotation.z;
    float xy = rotation.x * rotation.y;
    float xz = rotation.x * rotation.z;
    float yz = rotation.y * rotation.z;
    float wx = rotation.w * rotation.x;
    float wy = rotation.w * rotation.y;
    float wz = rotation.w * rotation.z;

    math::Matrix4 r;
    r.m[0][0] = 1.0f - 2.0f * (yy + zz);
    r.m[0][1] = 2.0f * (xy + wz);
    r.m[0][2] = 2.0f * (xz - wy);
    r.m[0][3] = 0.0f;

    r.m[1][0] = 2.0f * (xy - wz);
    r.m[1][1] = 1.0f - 2.0f * (xx + zz);
    r.m[1][2] = 2.0f * (yz + wx);
    r.m[1][3] = 0.0f;

    r.m[2][0] = 2.0f * (xz + wy);
    r.m[2][1] = 2.0f * (yz - wx);
    r.m[2][2] = 1.0f - 2.0f * (xx + yy);
    r.m[2][3] = 0.0f;

    r.m[3][0] = 0.0f;
    r.m[3][1] = 0.0f;
    r.m[3][2] = 0.0f;
    r.m[3][3] = 1.0f;

    math::Matrix4 s = math::Matrix4::Scale(scale);
    return t * r * s;
}

math::Matrix4 Camera::GetViewMatrix(const math::Matrix4& worldMatrix) const
{
    return worldMatrix.Inverted();
}

math::Matrix4 Camera::GetProjectionMatrix(float aspectRatio) const
{
    return math::Matrix4::Perspective(math::ToRadians(fovY), aspectRatio, nearPlane, farPlane);
}

}

}
}
