#pragma once

#include <sgkit/math/Quaternion.h>
#include <sgkit/math/Matrix4.h>
#include <sgkit/scene/Entity.h>

#include <memory>
#include <string>
#include <vector>

namespace sgkit {
namespace scene {

class Mesh;

namespace component {

// -- Transform

class Transform
{
public:
    math::Vector3    position { 0.0f, 0.0f, 0.0f };
    math::Quaternion rotation {};
    math::Vector3    scale    { 1.0f, 1.0f, 1.0f };

    Entity parent;
    std::vector<Entity> children;

    math::Matrix4 GetLocalMatrix() const;
};

// -- Camera

class Camera
{
public:
    float fovY      = 60.0f;
    float nearPlane = 0.1f;
    float farPlane  = 1000.0f;

    math::Matrix4 GetViewMatrix(const math::Matrix4& worldMatrix) const;
    math::Matrix4 GetProjectionMatrix(float aspectRatio) const;
};

// -- Light

class Light
{
public:
    enum class Type
    {
        Directional, Point, SpotLight
    } type = Type::Point;
    math::Vector3 direction { 0.0f, -1.0f, 0.0f };
    float cutOff      = 0.91f;
    float outerCutOff = 0.82f;

    math::Vector3 ambient   { 0.2f, 0.2f, 0.2f };
    math::Vector3 diffuse   { 0.5f, 0.5f, 0.5f };
    math::Vector3 specular  { 1.0f, 1.0f, 1.0f };

    float constant  = 1.0f;
    float linear    = 0.09f;
    float quadratic = 0.032f;
};

// -- MeshRenderer

class MeshRenderer
{
public:
    std::shared_ptr<Mesh> mesh;
    bool enabled = true;
};

}

}
}
