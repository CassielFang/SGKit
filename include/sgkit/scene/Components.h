#pragma once

#include <sgkit/scene/Entity.h>
#include <sgkit/math/Transform.h>
#include <sgkit/math/Vector3.h>
#include <sgkit/math/Matrix4.h>
#include <sgkit/graphics/Mesh.h>

#include <memory>
#include <vector>

namespace sgkit {
namespace scene {

struct Transform
{
    math::Vector3    position{0.0f, 0.0f, 0.0f};
    math::Quaternion rotation{};
    math::Vector3    scale{1.0f, 1.0f, 1.0f};

    Entity parent = k_InvalidEntity;
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

struct Camera
{
    float fovY      = 60.0f;
    float nearPlane = 0.1f;
    float farPlane  = 1000.0f;

    math::Matrix4 GetViewMatrix(const math::Matrix4& worldMatrix) const
    {
        return worldMatrix.Inverse();
    }

    math::Matrix4 GetProjectionMatrix(float aspectRatio) const
    {
        return math::Matrix4::Perspective(
            math::ToRadians(fovY), aspectRatio, nearPlane, farPlane);
    }
};

enum class LightType { k_Directional, k_Point };

struct Light
{
    LightType type      = LightType::k_Directional;
    math::Vector3 color{1.0f, 1.0f, 1.0f};
    float intensity     = 1.0f;
    float range         = 10.0f;
};

struct MeshRenderer
{
    std::shared_ptr<graphics::Mesh> mesh;
    bool enabled = true;
};

} // namespace scene
} // namespace sgkit
