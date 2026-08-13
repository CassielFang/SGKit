#include <sgkit/scene/DirectionalShadow.h>

#include <sgkit/scene/RenderQueue.h>
#include <sgkit/core/Window.h>
#include <glad/glad.h>

#include <cmath>

namespace sgkit {
namespace scene {

void DirectionalShadow::LazyInit()
{
    if (m_ready)
    {
        return;
    }

    m_fbo.Create(kResolution, kResolution);
    m_depthShader.LoadFromFile(
        "assets/shaders/shadow_depth.vert",
        "assets/shaders/shadow_depth.frag");
    m_shadowTex = m_fbo.GetDepthTexture();

    m_ready = true;
}

void DirectionalShadow::RenderPass(const RenderQueue& queue, const math::Vector3& lightDir)
{
    LazyInit();

    // Light space matrix: orthographic, centered on the scene origin.
    math::Vector3 lightDirN = lightDir.Normalized();
    math::Vector3 up{0.f, 1.f, 0.f};
    if (std::abs(math::Vector3::Dot(lightDirN, up)) > 0.999f)
    {
        up = {1.f, 0.f, 0.f};
    }

    math::Vector3 lightPos = -lightDirN * kLightDist;
    math::Matrix4 lightView = math::Matrix4::LookAt(lightPos, {0.f, 0.f, 0.f}, up);
    math::Matrix4 lightProj = math::Matrix4::Orthographic(
        -kOrthoHalf, kOrthoHalf, -kOrthoHalf, kOrthoHalf, kNear, kFar);
    m_lightMatrix = lightProj * lightView;

    // Render depth from the light's perspective.
    m_fbo.Bind();
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    // Render back faces only (front-face culling): closed meshes get acne-free
    // shadows via their own thickness. Per LearnOpenGL's shadow mapping guide.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glViewport(0, 0, kResolution, kResolution);
    glClear(GL_DEPTH_BUFFER_BIT);

    m_depthShader.Bind();
    m_depthShader.SetMatrix4("u_LightSpaceMatrix", m_lightMatrix);
    for (auto& batch : queue.GetOpaqueBatches())
    {
        if (!batch.vertexArray)
        {
            continue;
        }
        batch.vertexArray->Bind();
        for (auto& inst : batch.instances)
        {
            m_depthShader.SetMatrix4("u_Model", inst.modelMatrix);
            batch.vertexArray->Draw();
        }
    }

    glCullFace(GL_BACK); // restore default cull face for the main pass
    m_fbo.Unbind();
    core::Window& w = core::Window::instance();
    glViewport(0, 0, w.GetWidth(), w.GetHeight());
}

void DirectionalShadow::ApplyToShader(graphics::Shader& shader) const
{
    if (!m_shadowTex)
    {
        return;
    }

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, m_shadowTex);
    shader.SetInt("u_ShadowMap", 6);
    shader.SetMatrix4("u_LightSpaceMatrix", m_lightMatrix);
}

uint32_t DirectionalShadow::GetShadowTex() const
{
    return m_shadowTex;
}

}
}
