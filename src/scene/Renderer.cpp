#include <sgkit/scene/Renderer.h>

#include <sgkit/scene/Components.h>
#include <sgkit/core/DebugOut.h>
#include <glad/glad.h>

#include <cstdio>

static sgkit::scene::Renderer* g_Renderer = nullptr;

namespace sgkit {
namespace scene {

void Renderer::Create()
{
    if (g_Renderer) return;
    g_Renderer = new Renderer;
    core::DebugOut("[ SGKit Renderer ]: module created.");
}

void Renderer::Destroy()
{
    if (!g_Renderer) return;
    delete g_Renderer;
    g_Renderer = nullptr;
    core::DebugOut("[ SGKit Renderer ]: module destroyed.");
}

Renderer& Renderer::instance()
{
    return *g_Renderer;
}

void Renderer::SetClearColor(const math::Vector4& color)
{
    glClearColor(color.x, color.y, color.z, color.w);
}

void Renderer::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void Renderer::SetWireframe(bool enabled)
{
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
}

void Renderer::SetDepthTest(bool enabled)
{
    if (enabled) glEnable(GL_DEPTH_TEST);
    else         glDisable(GL_DEPTH_TEST);
}

void Renderer::SetBlend(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

void Renderer::SetCullFace(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

// -- Frame data

void Renderer::SetViewProjection(const math::Matrix4& vp)
{
    m_viewProjection = vp;
}

void Renderer::SetCameraPosition(const math::Vector3& pos)
{
    m_cameraPos = pos;
}

void Renderer::SetAmbientLight(const math::Vector3& color)
{
    m_ambientLight = color;
}

void Renderer::SetLights(const std::vector<LightInstance>& instances)
{
    m_lights = instances;
}

// -- Execute

void Renderer::Execute(const RenderQueue& queue)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    for (auto& batch : queue.GetOpaqueBatches())
        ExecuteBatch(batch);

    if (queue.HasTransparentBatches())
    {
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (auto& batch : queue.GetTransparentBatches())
            ExecuteBatch(batch);

        glDepthMask(GL_TRUE);
    }
}

void Renderer::ExecuteBatch(const RenderBatch& batch)
{
    if (!batch.shader || !batch.material || !batch.vertexArray || batch.instances.empty())
        return;

    auto& shader = *batch.shader;
    auto& mat    = *batch.material;
    auto& vao    = *batch.vertexArray;

    ApplyBatchState(batch);
    SetFrameUniforms(shader);

    if (mat.diffuse)
    {
        shader.SetInt("u_Material.diffuse", mat.diffuse->GetSlot());
        mat.diffuse->Bind();
    }
    if (mat.specular)
    {
        shader.SetInt("u_Material.specular", mat.specular->GetSlot());
        mat.specular->Bind();
    }
    shader.SetFloat("u_Material.shininess", mat.shininess);

    for (auto& inst : batch.instances)
    {
        shader.SetMatrix4("u_Model", inst.modelMatrix);
        vao.Draw();
    }

    //shader.Unbind();
    //vao.Unbind();
}

void Renderer::ApplyBatchState(const RenderBatch& batch)
{
    batch.vertexArray->Bind();
    batch.shader->Bind();
    

    auto& mat = *batch.material;

    switch (mat.cullMode)
    {
    case CullMode::Back:  glEnable(GL_CULL_FACE); glCullFace(GL_BACK);  break;
    case CullMode::Front: glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); break;
    case CullMode::None:  glDisable(GL_CULL_FACE);                      break;
    }

    switch (mat.blendMode)
    {
    case BlendMode::Opaque:     glDisable(GL_BLEND);                                    break;
    case BlendMode::AlphaBlend: glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
    case BlendMode::Additive:   glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);  break;
    }

    switch (mat.depthMode)
    {
    case DepthMode::ReadWrite: glDepthMask(GL_TRUE);  glEnable(GL_DEPTH_TEST); break;
    case DepthMode::ReadOnly:  glDepthMask(GL_FALSE); glEnable(GL_DEPTH_TEST); break;
    case DepthMode::None:      glDepthMask(GL_FALSE); glDisable(GL_DEPTH_TEST); break;
    }
}

void Renderer::SetFrameUniforms(graphics::Shader& shader)
{
    shader.SetMatrix4("u_ViewProjection", m_viewProjection);
    shader.SetVector3("u_cameraPos", m_cameraPos);

    //int count = static_cast<int>(m_lights.size());
    //shader.SetInt("u_LightCount", count);

    //for (int i = 0; i < count; ++i)
    //{
    //    const LightInstance& li = m_lights[i];
    //    char buf[64];
    //    std::snprintf(buf, sizeof(buf), "u_Lights[%d].position", i);
    //    shader.SetVector3(buf, li.worldPosition);
    //    std::snprintf(buf, sizeof(buf), "u_Lights[%d].ambient", i);
    //    shader.SetVector3(buf, li.attribute->ambient);
    //    std::snprintf(buf, sizeof(buf), "u_Lights[%d].diffuse", i);
    //    shader.SetVector3(buf, li.attribute->diffuse);
    //    std::snprintf(buf, sizeof(buf), "u_Lights[%d].specular", i);
    //    shader.SetVector3(buf, li.attribute->specular);
    //}
    shader.SetVector3("u_Light.position", m_lights[0].worldPosition);
    shader.SetVector3("u_Light.ambient", m_lights[0].attribute->ambient);
    shader.SetVector3("u_Light.diffuse", m_lights[0].attribute->diffuse);
    shader.SetVector3("u_Light.specular", m_lights[0].attribute->specular);
}

}
}
