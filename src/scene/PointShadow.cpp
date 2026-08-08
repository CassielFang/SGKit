#include <sgkit/scene/PointShadow.h>

#include <sgkit/scene/RenderQueue.h>
#include <sgkit/core/Window.h>
#include <glad/glad.h>

namespace sgkit {
namespace scene {

PointShadow::PointShadow()  = default;

PointShadow::~PointShadow()
{
    for (int i = 0; i < kMaxLights; ++i)
        if (m_cubemaps[i]) { glDeleteTextures(1, &m_cubemaps[i]); m_cubemaps[i] = 0; }
}

void PointShadow::LazyInit()
{
    if (m_ready) return;

    m_depthShader.LoadFromFile(
        "assets/shaders/point_shadow_depth.vert",
        "assets/shaders/point_shadow_depth.frag",
        "assets/shaders/point_shadow_depth.geom");

    // Reusable FBO for depth cubemap rendering
    glGenFramebuffers(1, &m_fbo);
    glGenRenderbuffers(1, &m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          kResolution, kResolution);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    m_ready = true;
}

bool PointShadow::HasAny() const
{
    for (int i = 0; i < kMaxLights; ++i)
        if (m_cubemaps[i] != 0) return true;
    return false;
}

void PointShadow::RenderPass(const RenderQueue& queue,
                              const math::Vector3 lightPositions[kMaxLights],
                              int activeCount, float /*farPlane*/)
{
    LazyInit();
    if (activeCount <= 0) return;

    // -- Ensure cubemaps exist
    for (int i = 0; i < activeCount; ++i)
    {
        if (m_cubemaps[i] == 0)
        {
            glGenTextures(1, &m_cubemaps[i]);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemaps[i]);
            for (int f = 0; f < 6; ++f)
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0,
                             GL_DEPTH_COMPONENT, kResolution, kResolution,
                             0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }
    }

    // -- 6 face shadow matrices (view from origin, GS translates by -lightPos)
    math::Matrix4 shadowProj = math::Matrix4::Perspective(
        math::ToRadians(90.0f), 1.0f, 1.0f, kFarPlane);

    struct Face { math::Vector3 target; math::Vector3 up; };
    const Face faces[6] = {
        {{ 1, 0, 0}, {0,-1, 0}}, {{-1, 0, 0}, {0,-1, 0}},
        {{ 0, 1, 0}, {0, 0, 1}}, {{ 0,-1, 0}, {0, 0,-1}},
        {{ 0, 0, 1}, {0,-1, 0}}, {{ 0, 0,-1}, {0,-1, 0}},
    };

    math::Matrix4 shadowMatrices[6];
    for (int f = 0; f < 6; ++f)
    {
        math::Matrix4 shadowView = math::Matrix4::LookAt(
            math::Vector3{0,0,0}, faces[f].target, faces[f].up);
        shadowMatrices[f] = shadowProj * shadowView;
    }

    // -- Setup FBO + state
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, m_rbo);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glViewport(0, 0, kResolution, kResolution);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);

    m_depthShader.Bind();
    for (int f = 0; f < 6; ++f)
    {
        char name[32];
        snprintf(name, 32, "u_ShadowMatrices[%d]", f);
        m_depthShader.SetMatrix4(name, shadowMatrices[f]);
    }
    m_depthShader.SetFloat("u_FarPlane", kFarPlane);

    // -- Render each light into its cubemap
    for (int li = 0; li < activeCount; ++li)
    {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                             m_cubemaps[li], 0);
        glClear(GL_DEPTH_BUFFER_BIT);

        m_depthShader.SetVector3("u_LightPos", lightPositions[li]);

        for (auto& batch : queue.GetOpaqueBatches())
        {
            if (!batch.vertexArray) continue;
            batch.vertexArray->Bind();
            for (auto& inst : batch.instances)
            {
                m_depthShader.SetMatrix4("u_Model", inst.modelMatrix);
                batch.vertexArray->Draw();
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_CULL_FACE);

    core::Window& w = core::Window::instance();
    glViewport(0, 0, w.GetWidth(), w.GetHeight());
}

void PointShadow::ApplyToShader(graphics::Shader& shader, int baseUnit) const
{
    shader.SetInt("u_PointShadows", HasAny() ? 1 : 0);
    if (!HasAny()) return;

    for (int i = 0; i < kMaxLights; ++i)
    {
        char name[32];
        if (m_cubemaps[i])
        {
            snprintf(name, 32, "u_PointShadowEnabled[%d]", i);
            shader.SetInt(name, 1);

            snprintf(name, 32, "u_PointShadowMap[%d]", i);
            shader.SetInt(name, baseUnit + i);
            glActiveTexture(GL_TEXTURE0 + baseUnit + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemaps[i]);

            snprintf(name, 32, "u_PointShadowFar[%d]", i);
            shader.SetFloat(name, kFarPlane);
        }
        else
        {
            snprintf(name, 32, "u_PointShadowEnabled[%d]", i);
            shader.SetInt(name, 0);
        }
    }
}

}
}
