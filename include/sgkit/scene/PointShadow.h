#pragma once

#include <sgkit/graphics/Shader.h>
#include <sgkit/math/Matrix4.h>
#include <sgkit/math/Vector3.h>

#include <cstdint>

namespace sgkit {
namespace scene {

class RenderQueue;

// Omnidirectional shadow maps for point lights.
// Each point light (up to 4) gets a 1024² depth cubemap rendered
// in one pass via geometry shader (gl_Layer).
// Owned by Renderer; not a standalone singleton.
class PointShadow
{
public:
    static constexpr int   kMaxLights  = 4;
    static constexpr int   kResolution = 2048;
    static constexpr float kFarPlane   = 15.0f;

    PointShadow();
    ~PointShadow();

    PointShadow(const PointShadow&) = delete;
    PointShadow& operator=(const PointShadow&) = delete;
    PointShadow(PointShadow&&) = delete;
    PointShadow& operator=(PointShadow&&) = delete;

    // Render depth cubemaps for all active point lights.
    void RenderPass(const RenderQueue& queue,
                    const math::Vector3 lightPositions[kMaxLights],
                    int activeCount, float farPlane);

    // Apply point-shadow uniforms to a shader.
    // Cubemaps are at consecutive texture units starting from `baseUnit`.
    void ApplyToShader(graphics::Shader& shader, int baseUnit) const;

    // Query
    bool     HasAny() const;
    uint32_t GetCubemap(int i) const { return m_cubemaps[i]; }

private:
    uint32_t         m_cubemaps[kMaxLights] = {};
    uint32_t         m_fbo   = 0;
    uint32_t         m_rbo   = 0;
    graphics::Shader m_depthShader;
    bool             m_ready = false;

    void LazyInit();
};

}
}
