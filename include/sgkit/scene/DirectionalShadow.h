#pragma once

#include <sgkit/graphics/FrameBuffer.h>
#include <sgkit/graphics/Shader.h>
#include <sgkit/math/Matrix4.h>
#include <sgkit/math/Vector3.h>

#include <cstdint>

namespace sgkit {
namespace scene {

class RenderQueue;

// Directional-light shadow map: a single orthographic depth map rendered from
// the light's point of view. Follows LearnOpenGL's shadow mapping approach.
class DirectionalShadow
{
public:
    DirectionalShadow() = default;
    ~DirectionalShadow() = default;

    DirectionalShadow(const DirectionalShadow&) = delete;
    DirectionalShadow& operator=(const DirectionalShadow&) = delete;
    DirectionalShadow(DirectionalShadow&&) = delete;
    DirectionalShadow& operator=(DirectionalShadow&&) = delete;

    static constexpr int   kResolution = 4096;
    static constexpr float kOrthoHalf  = 15.0f; // ortho half-extent (world units)
    static constexpr float kLightDist  = 30.0f; // light distance from origin
    static constexpr float kNear       = 1.0f;
    static constexpr float kFar        = 40.0f;

    // Render the depth-only shadow map for one directional light.
    void RenderPass(const RenderQueue& queue, const math::Vector3& lightDir);

    // Apply shadow uniforms to a shader (shadow map at texture unit 6).
    void ApplyToShader(graphics::Shader& shader) const;

    uint32_t GetShadowTex() const;

private:
    math::Matrix4         m_lightMatrix = {};
    uint32_t              m_shadowTex = 0;
    graphics::FrameBuffer m_fbo;
    graphics::Shader      m_depthShader;
    bool                  m_ready = false;

    void LazyInit();
};

}
}
