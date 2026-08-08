#pragma once

#include <sgkit/graphics/Shader.h>
#include <sgkit/graphics/VertexArray.h>
#include <sgkit/graphics/VertexBuffer.h>
#include <sgkit/graphics/IndexBuffer.h>
#include <sgkit/math/Matrix4.h>

#include <string>
#include <cstdint>

namespace sgkit {
namespace scene {

// Skybox / environment cubemap renderer.
// Owned by Renderer; not a standalone singleton.
class SkyboxRenderer
{
public:
    SkyboxRenderer() = default;
    ~SkyboxRenderer() = default;

    SkyboxRenderer(const SkyboxRenderer&) = delete;
    SkyboxRenderer& operator=(const SkyboxRenderer&) = delete;
    SkyboxRenderer(SkyboxRenderer&&) = delete;
    SkyboxRenderer& operator=(SkyboxRenderer&&) = delete;

    // Load an equirectangular .hdr, convert to cubemap, and prepare the
    // skybox shader.  Returns false if shaders or the HDR are missing.
    bool Setup(const std::string& hdrPath);

    // Draw the skybox.  Call AFTER scene rendering so it fills only empty
    // pixels (depth == far plane).  The view matrix should have its
    // translation already stripped.
    void Render(const math::Matrix4& view, const math::Matrix4& proj);

    // Release the cubemap texture.
    void Destroy();

    bool IsReady() const;

private:
    graphics::Shader      m_shader;
    graphics::Shader      m_equiConvShader;
    graphics::VertexBuffer m_vb;
    graphics::IndexBuffer  m_ib;
    graphics::VertexArray  m_vao;
    uint32_t               m_cubemap = 0;
    bool                   m_ready   = false;

    void GenerateCubemap(const std::string& hdrPath);
};

}
}
