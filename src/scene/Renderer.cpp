#include <sgkit/scene/Renderer.h>

#include <sgkit/scene/Components.h>
#include <sgkit/framework/DebugOut.h>
#include <glad/glad.h>

static sgkit::scene::Renderer* g_Renderer = nullptr;

namespace sgkit {
namespace scene {

void Renderer::Create()
{
    if (g_Renderer)
    {

    }
    g_Renderer = new Renderer;
    glEnable(GL_MULTISAMPLE);
    SGK_LOG_INFO("Renderer", "Module created");
}

void Renderer::Destroy()
{
    if (!g_Renderer)
    {
        return;
    }
    delete g_Renderer;
    g_Renderer = nullptr;
    SGK_LOG_INFO("Renderer", "Module destroyed");
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
    if (enabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
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
    memcpy(m_frameData.viewProjection, vp.Data(), 16 * sizeof(float));
}

void Renderer::SetCameraPosition(const math::Vector3& pos)
{
    m_cameraPos = pos;
    m_frameData.cameraPos[0] = pos.x;
    m_frameData.cameraPos[1] = pos.y;
    m_frameData.cameraPos[2] = pos.z;
    m_frameData.cameraPos[3] = 0.0f;
}

void Renderer::SetCameraForward(const math::Vector3& fwd)
{
    m_cameraForward = fwd;
    m_frameData.cameraForward[0] = fwd.x;
    m_frameData.cameraForward[1] = fwd.y;
    m_frameData.cameraForward[2] = fwd.z;
    m_frameData.cameraForward[3] = 0.0f;
}

void Renderer::SetLights(const std::vector<LightInstance>& instances)
{
    int dC = 0, pC = 0, sC = 0;
    for (auto& li : instances)
    {
        auto& l = *li.attribute;
        switch (l.type)
        {
        case component::Light::Type::Directional:
            dC = 1;
            m_frameData.dirDirection[0]=l.direction.x; m_frameData.dirDirection[1]=l.direction.y; m_frameData.dirDirection[2]=l.direction.z;
            m_frameData.dirAmbient[0]=l.ambient.x; m_frameData.dirAmbient[1]=l.ambient.y; m_frameData.dirAmbient[2]=l.ambient.z;
            m_frameData.dirDiffuse[0]=l.diffuse.x; m_frameData.dirDiffuse[1]=l.diffuse.y; m_frameData.dirDiffuse[2]=l.diffuse.z;
            m_frameData.dirSpecular[0]=l.specular.x; m_frameData.dirSpecular[1]=l.specular.y; m_frameData.dirSpecular[2]=l.specular.z;
            break;
        case component::Light::Type::Point:
            if (pC < 4)
            {
                auto& p = m_frameData.pointLights[pC++];
                p.position[0]=li.worldPosition.x; p.position[1]=li.worldPosition.y; p.position[2]=li.worldPosition.z;
                p.ambient[0]=l.ambient.x; p.ambient[1]=l.ambient.y; p.ambient[2]=l.ambient.z;
                p.diffuse[0]=l.diffuse.x; p.diffuse[1]=l.diffuse.y; p.diffuse[2]=l.diffuse.z;
                p.specular[0]=l.specular.x; p.specular[1]=l.specular.y; p.specular[2]=l.specular.z;
                p.attenuation[0]=l.constant; p.attenuation[1]=l.linear; p.attenuation[2]=l.quadratic;
            }
            break;
        case component::Light::Type::SpotLight:
            if (sC < 4)
            {
                auto& s = m_frameData.spotLights[sC++];
                s.position[0]=li.worldPosition.x; s.position[1]=li.worldPosition.y; s.position[2]=li.worldPosition.z;
                s.direction[0]=l.direction.x; s.direction[1]=l.direction.y; s.direction[2]=l.direction.z;
                s.ambient[0]=l.ambient.x; s.ambient[1]=l.ambient.y; s.ambient[2]=l.ambient.z;
                s.diffuse[0]=l.diffuse.x; s.diffuse[1]=l.diffuse.y; s.diffuse[2]=l.diffuse.z;
                s.specular[0]=l.specular.x; s.specular[1]=l.specular.y; s.specular[2]=l.specular.z;
                s.attenCut[0]=l.constant; s.attenCut[1]=l.linear; s.attenCut[2]=l.quadratic; s.attenCut[3]=l.cutOff;
                s.outerCutPad[0]=l.outerCutOff;
            }
            break;
        }
    }
    m_frameData.lightCounts[0] = dC;
    m_frameData.lightCounts[1] = pC;
    m_frameData.lightCounts[2] = sC;
}

void Renderer::CommitFrameData()
{
    UpdateFrameUBO();
}

// -- Directional shadow (delegate)

void Renderer::RenderDirectionalShadowPass(
    const RenderQueue& queue, const math::Vector3& lightDir)
{
    m_dirShadow.RenderPass(queue, lightDir);
}

// -- Skybox (delegate)

bool Renderer::SetupSkybox(const std::string& hdrPath)
{
    return m_skybox.Setup(hdrPath);
}

void Renderer::SetSkyboxMatrices(const math::Matrix4& view, const math::Matrix4& proj)
{
    m_skyboxView = view;
    m_skyboxProj = proj;
}

void Renderer::DestroySkybox()
{
    m_skybox.Destroy();
}

// -- Execute

void Renderer::Execute(const RenderQueue& queue)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // 1. Opaque pass
    for (auto& batch : queue.GetOpaqueBatches())
    {
        ExecuteBatch(batch);
    }

    // 2. Skybox - between opaque and transparent so additive markers stay visible
    m_skybox.Render(m_skyboxView, m_skyboxProj);

    // 3. Transparent pass
    if (queue.HasTransparentBatches())
    {
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (auto& batch : queue.GetTransparentBatches())
        {
            ExecuteBatch(batch);
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

void Renderer::ExecuteBatch(const RenderBatch& batch)
{
    if (!batch.shader || !batch.material || !batch.vertexArray || batch.instances.empty())
    {
        return;
    }

    auto& shader = *batch.shader;
    auto& mat    = *batch.material;
    auto& vao    = *batch.vertexArray;

    ApplyBatchState(batch);
    SetFrameUniforms(shader);

    if (mat.lightingModel == LightingModel::PBR)
    {
        shader.SetInt("u_PBR_combinedMR", mat.pbrCombinedMetallicRoughness ? 1 : 0);
        shader.SetFloat("u_PBR_metallicFactor",  mat.metallicFactor);
        shader.SetFloat("u_PBR_roughnessFactor", mat.roughnessFactor);
        shader.SetFloat("u_PBR_alphaFactor",     mat.alphaFactor);
        shader.SetFloat("u_PBR_alphaCutoff",    mat.alphaCutoff);
        shader.SetFloat("u_PBR_normalScale",    mat.normalScale);
        shader.SetFloat("u_PBR_aoStrength",     mat.aoStrength);

        if (mat.albedo)    { shader.SetInt("u_PBR_albedo", mat.albedo->GetSlot());       mat.albedo->Bind(); }
        if (mat.metallic)  { shader.SetInt("u_PBR_metallic", mat.metallic->GetSlot());   mat.metallic->Bind(); }
        if (mat.roughness) { shader.SetInt("u_PBR_roughness", mat.roughness->GetSlot()); mat.roughness->Bind(); }
        if (mat.normalMap) { shader.SetInt("u_PBR_normal", mat.normalMap->GetSlot());    mat.normalMap->Bind(); }
        if (mat.ao)        { shader.SetInt("u_PBR_ao", mat.ao->GetSlot());               mat.ao->Bind(); }
        if (mat.emissive) 
        {
            shader.SetInt("u_PBR_emissive", mat.emissive->GetSlot());
            shader.SetVector3("u_PBR_emissiveFactor", mat.emissiveFactor);
            mat.emissive->Bind();
        }
        else
        {
            shader.SetVector3("u_PBR_emissiveFactor", math::Vector3{});
        }
    }
    else
    {
        if (mat.diffuse)  { shader.SetInt("u_Material.diffuse", mat.diffuse->GetSlot());   mat.diffuse->Bind(); }
        if (mat.specular) { shader.SetInt("u_Material.specular", mat.specular->GetSlot()); mat.specular->Bind(); }
        shader.SetFloat("u_Material.shininess", mat.shininess);
    }

    if (batch.instances.size() > 1)
    {
        char name[32];
        shader.SetInt("u_Instanced", 1);
        for (size_t i = 0; i < batch.instances.size(); ++i)
        {
            snprintf(name, 32, "u_ModelMatrices[%zu]", i);
            shader.SetMatrix4(name, batch.instances[i].modelMatrix);
        }
        vao.DrawInstanced(static_cast<uint32_t>(batch.instances.size()));
        shader.SetInt("u_Instanced", 0);
    }
    else
    {
        for (auto& inst : batch.instances)
        {
            shader.SetMatrix4("u_Model", inst.modelMatrix);
            vao.Draw();
        }
    }
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

void Renderer::EnsureUBO()
{
    if (!m_uboReady)
    {
        m_frameUBO.Create(sizeof(FrameUniforms), 0);
        m_uboReady = true;
    }
}

void Renderer::UpdateFrameUBO()
{
    EnsureUBO();
    m_frameUBO.Upload(&m_frameData, sizeof(FrameUniforms));
}

void Renderer::SetFrameUniforms(graphics::Shader& shader)
{
    shader.SetInt("u_ShadowsEnabled", m_dirShadow.GetShadowTex() ? 1 : 0);
    if (m_dirShadow.GetShadowTex())
    {
        m_dirShadow.ApplyToShader(shader);
    }
}

}
}
