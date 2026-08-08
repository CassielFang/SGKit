#pragma once

#include <sgkit/graphics/Framebuffer.h>
#include <sgkit/graphics/Shader.h>
#include <sgkit/math/Matrix4.h>
#include <sgkit/math/Vector3.h>

#include <cstdint>

namespace sgkit {
namespace scene {

class RenderQueue;

// Cascaded Shadow Maps - 3-cascade directional-light shadow atlas.
// Owned by Renderer; not a standalone singleton.
class CSMShadow
{
public:
    CSMShadow() = default;
    ~CSMShadow() = default;

    CSMShadow(const CSMShadow&) = delete;
    CSMShadow& operator=(const CSMShadow&) = delete;
    CSMShadow(CSMShadow&&) = delete;
    CSMShadow& operator=(CSMShadow&&) = delete;

    static constexpr int kCascades   = 3;
    static constexpr int kResolution = 4096;

    // Render the depth-only shadow atlas for one directional light.
    void RenderPass(
        const RenderQueue& queue, const math::Vector3& lightDir,
        const math::Matrix4& camView, const math::Matrix4& camProj,
        const math::Vector3& cameraPos,
        float cameraNear, float cameraFar, float aspect);

    // Apply CSM uniforms to a shader (shadow atlas at unit 6).
    void ApplyToShader(graphics::Shader& shader) const;

    // Query
    bool IsReady() const;
    uint32_t GetShadowTex() const;
    const math::Matrix4* LightMatrices() const;
    const float* SplitDepths() const;

private:
    math::Matrix4         m_lightMatrices[kCascades] = {};
    float                 m_splitDepths[kCascades]   = {};
    uint32_t              m_shadowTex = 0;
    graphics::FrameBuffer m_fbo;
    graphics::Shader      m_depthShader;
    bool                  m_ready = false;

    void LazyInit();
};

}
}
