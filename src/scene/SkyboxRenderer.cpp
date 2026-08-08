#include <sgkit/scene/SkyboxRenderer.h>

#include <sgkit/framework/DebugOut.h>
#include <sgkit/graphics/Texture.h>
#include <glad/glad.h>

namespace sgkit {
namespace scene {

bool SkyboxRenderer::Setup(const std::string& hdrPath)
{
    Destroy();

    if (!m_shader.LoadFromFile(
            "assets/shaders/skybox.vert",
            "assets/shaders/skybox.frag"))
    {
        SGK_LOG_ERROR("Skybox", "Failed to load skybox shader");
        return false;
    }
    if (!m_equiConvShader.LoadFromFile(
            "assets/shaders/equirect_to_cubemap.vert",
            "assets/shaders/equirect_to_cubemap.frag"))
    {
        SGK_LOG_ERROR("Skybox", "Failed to load equirect conversion shader");
        return false;
    }

    // Position-only cube for rendering into cubemap faces
    constexpr float cubeVerts[] = {
        -1,-1, 1,  1,-1, 1,  1, 1, 1, -1, 1, 1,
         1,-1,-1, -1,-1,-1, -1, 1,-1,  1, 1,-1,
        -1, 1, 1,  1, 1, 1,  1, 1,-1, -1, 1,-1,
        -1,-1,-1,  1,-1,-1,  1,-1, 1, -1,-1, 1,
         1,-1, 1,  1,-1,-1,  1, 1,-1,  1, 1, 1,
        -1,-1,-1, -1,-1, 1, -1, 1, 1, -1, 1,-1,
    };
    constexpr uint32_t cubeIdx[] = {
         0, 1, 2, 2, 3, 0,    4, 5, 6, 6, 7, 4,
         8, 9,10,10,11, 8,   12,13,14,14,15,12,
        16,17,18,18,19,16,   20,21,22,22,23,20,
    };

    graphics::VertexLayout lo;
    lo.PushFloat(0, 3);
    m_vb.Create(cubeVerts, sizeof(cubeVerts));
    m_ib.Create(cubeIdx, 36);
    m_vao.Create();
    m_vao.SetVertexBuffer(
        std::make_shared<graphics::VertexBuffer>(std::move(m_vb)), lo);
    m_vao.SetIndexBuffer(
        std::make_shared<graphics::IndexBuffer>(std::move(m_ib)));

    GenerateCubemap(hdrPath);

    m_ready = true;
    SGK_LOG_INFO("Skybox", "Ready");
    return true;
}

void SkyboxRenderer::GenerateCubemap(const std::string& hdrPath)
{
    constexpr int kRes = 512;

    graphics::Texture hdrTex(0);
    if (!hdrTex.LoadHDR(hdrPath))
    {
        SGK_LOG_ERROR("Skybox", "Cubemap generation failed: cannot load %s", hdrPath.c_str());
        return;
    }

    glGenTextures(1, &m_cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);
    for (int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     kRes, kRes, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    GLuint fbo = 0, rbo = 0;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kRes, kRes);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    math::Matrix4 proj = math::Matrix4::Perspective(
        math::ToRadians(90.0f), 1.0f, 0.1f, 10.0f);

    struct Face { math::Vector3 target; math::Vector3 up; };
    const Face faces[6] = {
        {{ 1, 0, 0}, {0,-1, 0}}, {{-1, 0, 0}, {0,-1, 0}},
        {{ 0, 1, 0}, {0, 0, 1}}, {{ 0,-1, 0}, {0, 0,-1}},
        {{ 0, 0, 1}, {0,-1, 0}}, {{ 0, 0,-1}, {0,-1, 0}},
    };

    m_equiConvShader.Bind();
    m_equiConvShader.SetInt("u_EquirectMap", 0);
    hdrTex.SetSlot(0);
    hdrTex.Bind();

    // Warm-up draw: let the driver compile the shader for this VAO layout
    // before the real cubemap faces, avoiding a mid-render recompilation warning.
    glViewport(0, 0, 1, 1);
    glScissor(0, 0, 0, 0);
    glEnable(GL_SCISSOR_TEST);
    m_equiConvShader.SetMatrix4("u_ViewProjection", proj *
        math::Matrix4::LookAt(math::Vector3{0,0,0}, math::Vector3{1,0,0}, math::Vector3{0,-1,0}));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X, m_cubemap, 0);
    m_vao.Draw();
    glDisable(GL_SCISSOR_TEST);

    glViewport(0, 0, kRes, kRes);
    glDisable(GL_CULL_FACE);

    for (int i = 0; i < 6; ++i)
    {
        math::Matrix4 view = math::Matrix4::LookAt(
            math::Vector3{0,0,0}, faces[i].target, faces[i].up);
        m_equiConvShader.SetMatrix4("u_ViewProjection", proj * view);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               m_cubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_vao.Draw();
    }

    glEnable(GL_CULL_FACE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
}

void SkyboxRenderer::Render(const math::Matrix4& view, const math::Matrix4& proj)
{
    if (!m_ready) return;

    math::Matrix4 viewNoTrans = view;
    viewNoTrans.m[3][0] = 0.f;
    viewNoTrans.m[3][1] = 0.f;
    viewNoTrans.m[3][2] = 0.f;

    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    m_shader.Bind();
    m_shader.SetMatrix4("u_ViewProjectionSky", proj * viewNoTrans);
    m_shader.SetInt("u_Skybox", 7);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);

    m_vao.Bind();
    m_vao.Draw();

    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

void SkyboxRenderer::Destroy()
{
    if (m_cubemap) { glDeleteTextures(1, &m_cubemap); m_cubemap = 0; }
    m_ready = false;
}

}
}
