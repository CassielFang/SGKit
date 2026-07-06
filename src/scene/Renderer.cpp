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

    int dCount = 0, pCount = 0, sCount = 0, count = static_cast<int>(m_lights.size());
    char buf[64]{};

    auto setupLightv = [&shader, &buf](const char* format, int index, math::Vector3& val)
        {
            std::snprintf(buf, 64, format, index);
            shader.SetVector3(buf, val);
        };
    auto setupLightf = [&shader, &buf](const char* format, int index, float val)
        {
            std::snprintf(buf, 64, format, index);
            shader.SetFloat(buf, val);
        };

    for (int i = 0; i < count; ++i)
    {
        LightInstance& l = m_lights[i];
        switch (m_lights[i].attribute->type)
        {
        case component::Light::Type::Directional:
        {
            shader.SetVector3("u_DirectionalLight.direction", l.attribute->direction);
            shader.SetVector3("u_DirectionalLight.ambient", l.attribute->ambient);
            shader.SetVector3("u_DirectionalLight.diffuse", l.attribute->diffuse);
            shader.SetVector3("u_DirectionalLight.specular", l.attribute->specular);
            dCount = 1;
            break;
        }
        case component::Light::Type::Point:
        {
            setupLightv("u_PointLights[%d].position", pCount, l.worldPosition);
            setupLightv("u_PointLights[%d].ambient", pCount, l.attribute->ambient);
            setupLightv("u_PointLights[%d].diffuse", pCount, l.attribute->diffuse);
            setupLightv("u_PointLights[%d].specular", pCount, l.attribute->specular);
            setupLightf("u_PointLights[%d].constant", pCount, l.attribute->constant);
            setupLightf("u_PointLights[%d].linear", pCount, l.attribute->linear);
            setupLightf("u_PointLights[%d].quadratic", pCount, l.attribute->quadratic);
            ++pCount;
            break;
        }
        case component::Light::Type::SpotLight:
        {
            setupLightv("u_SpotLights[%d].position", sCount, l.worldPosition);
            setupLightv("u_SpotLights[%d].direction", sCount, l.attribute->direction);
            setupLightf("u_SpotLights[%d].cutOff", sCount, l.attribute->cutOff);
            setupLightf("u_SpotLights[%d].outerCutOff", sCount, l.attribute->outerCutOff);
            setupLightv("u_SpotLights[%d].ambient", sCount, l.attribute->ambient);
            setupLightv("u_SpotLights[%d].diffuse", sCount, l.attribute->diffuse);
            setupLightv("u_SpotLights[%d].specular", sCount, l.attribute->specular);
            setupLightf("u_SpotLights[%d].constant", sCount, l.attribute->constant);
            setupLightf("u_SpotLights[%d].linear", sCount, l.attribute->linear);
            setupLightf("u_SpotLights[%d].quadratic", sCount, l.attribute->quadratic);
            ++sCount;
            break;
        }
        }
    }
    shader.SetInt("u_dLightCount", dCount);
    shader.SetInt("u_pLightCount", pCount);
    shader.SetInt("u_sLightCount", sCount);
}

}
}
