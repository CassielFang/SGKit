#pragma once

#include <sgkit/math/Matrix4.h>
#include <sgkit/math/Vector3.h>

namespace sgkit {
namespace graphics {

// GPU-ready light data extracted from scene::Light.
// Kept in graphics namespace so Mesh doesn't depend on scene types.
struct LightUniforms
{
    math::Vector3 position{0.0f, 0.0f, 0.0f};
    math::Vector3 ambient{0.2f, 0.2f, 0.2f};
    math::Vector3 diffuse{0.5f, 0.5f, 0.5f};
    math::Vector3 specular{1.0f, 1.0f, 1.0f};
};

// Per-frame drawing context.
// Built by Scene from camera / light components, consumed by Mesh::Render().
struct RenderContext
{
    math::Matrix4 viewProjection = math::Matrix4::Identity();
    math::Vector3 cameraPos{0.0f, 0.0f, 0.0f};
    LightUniforms light;
    bool hasLight = false;
};

}
}
