#pragma once

#include <sgkit/scene/Entity.h>
#include <sgkit/math/Transform.h>
#include <sgkit/math/Matrix4.h>
#include <sgkit/graphics/Mesh.h>

namespace sgkit {
namespace scene {

class Transform
{
public:
    math::Vector3    position{0.0f, 0.0f, 0.0f};
    math::Quaternion rotation{};
    math::Vector3    scale{1.0f, 1.0f, 1.0f};

    Entity parent;
    std::vector<Entity> children;

    math::Matrix4 GetLocalMatrix() const
    {
        math::Transform tf;
        tf.position = position;
        tf.rotation = rotation;
        tf.scale    = scale;
        return tf.GetLocalMatrix();
    }
};

class Camera
{
public:
    float fovY      = 60.0f;
    float nearPlane = 0.1f;
    float farPlane  = 1000.0f;

    math::Matrix4 GetViewMatrix(const math::Matrix4& worldMatrix) const
    {
        return worldMatrix.Inverted();
    }

    math::Matrix4 GetProjectionMatrix(float aspectRatio) const
    {
        return math::Matrix4::Perspective(math::ToRadians(fovY), aspectRatio, nearPlane, farPlane);
    }
};

class Light
{
public:
    math::Vector3 ambient{ 0.2f, 0.2f, 0.2f };
    math::Vector3 diffuse{ 0.5f, 0.5f, 0.5f };
    math::Vector3 specular{ 1.0f, 1.0f, 1.0f };
};

class MeshRenderer
{
public:
    std::shared_ptr<graphics::Mesh> mesh;
    bool enabled = true;
};

} // namespace scene
} // namespace sgkit
