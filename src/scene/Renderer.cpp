#include <sgkit/scene/Renderer.h>

#include <sgkit/scene/Components.h>
#include <sgkit/core/Window.h>
#include <sgkit/framework/DebugOut.h>
#include <glad/glad.h>

static sgkit::scene::Renderer* g_Renderer = nullptr;

namespace sgkit {
namespace scene {

void Renderer::Create()
{
    if (g_Renderer) return;
    g_Renderer = new Renderer;
    glEnable(GL_MULTISAMPLE);
    SGK_LOG_INFO("Renderer", "Module created");
}

void Renderer::Destroy()
{
    if (!g_Renderer) return;
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

void Renderer::SetLights(const std::vector<LightInstance>& instances)
{
    m_lights = instances;

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
            if (pC < 4) {
                auto& p = m_frameData.pointLights[pC++];
                p.position[0]=li.worldPosition.x; p.position[1]=li.worldPosition.y; p.position[2]=li.worldPosition.z;
                p.ambient[0]=l.ambient.x; p.ambient[1]=l.ambient.y; p.ambient[2]=l.ambient.z;
                p.diffuse[0]=l.diffuse.x; p.diffuse[1]=l.diffuse.y; p.diffuse[2]=l.diffuse.z;
                p.specular[0]=l.specular.x; p.specular[1]=l.specular.y; p.specular[2]=l.specular.z;
                p.attenuation[0]=l.constant; p.attenuation[1]=l.linear; p.attenuation[2]=l.quadratic;
            }
            break;
        case component::Light::Type::SpotLight:
            if (sC < 4) {
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
    memcpy(m_frameData.lightSpaceMatrix, m_csmLightMatrices[0].Data(), 16 * sizeof(float));
    m_frameData.lightCounts[3] = m_csmShadowTex ? 1 : 0;
    UpdateFrameUBO();
}

// -- CSM Shadow

void Renderer::SetCSMData()
{
    // Called by Scene before Execute. Shadow atlas is now on unit 6;
    // cascade uniforms are set in SetFrameUniforms per batch.
}

void Renderer::RenderCSMShadowPass(const RenderQueue& queue, const math::Vector3& lightDir,
                                    const math::Matrix4& camView, const math::Matrix4& camProj)
{
    // -- Lazy init
    if (!m_csmReady)
    {
        int atlasW = kCSMResolution * kCSMCascades;
        m_csmFBO.Create(atlasW, kCSMResolution);
        m_csmDepthShader.LoadFromFile(
            "assets/shaders/shadow_depth.vert",
            "assets/shaders/shadow_depth.frag");
        m_csmShadowTex = m_csmFBO.GetDepthTexture();
        m_csmReady = true;
    }

    m_csmFBO.Bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(4.0f, 4.0f);

    // -- Cascade split depths (practical split scheme, lambda=0.75)
    float nearP = 0.1f, farP = 50.0f;
    float lambda = 0.75f;
    for (int i = 0; i < kCSMCascades; ++i)
    {
        float p = float(i + 1) / kCSMCascades;
        float logS = nearP * pow(farP / nearP, p);
        float linS = nearP + (farP - nearP) * p;
        m_csmSplitDepths[i] = logS * lambda + linS * (1.0f - lambda);
    }

    // Inverse VP to get frustum corners in view space
    math::Matrix4 invCamVP = (camProj * camView);
    // ... no Inverse() available - compute corners another way

    // Compute frustum heights at each split in view space
    float fovHalf = 0.0f; // will use the split depths with fixed ortho for now
    // Simplified approach: fixed ortho per cascade, centred on camera look-at

    for (int c = 0; c < kCSMCascades; ++c)
    {
        float prevDist = (c == 0) ? nearP : m_csmSplitDepths[c - 1];
        float curDist  = m_csmSplitDepths[c];
        float midDist  = (prevDist + curDist) * 0.5f;

        // Approximate half-extent using typical FOV
        float halfExt = curDist * 0.8f;  // covers most of frustum at this distance

        math::Matrix4 lightProj = math::Matrix4::Orthographic(
            -halfExt, halfExt, -halfExt, halfExt, nearP, farP);
        math::Vector3 sceneCenter{0, 0, 0};
        math::Vector3 lightPos = sceneCenter - lightDir * (farP * 0.5f);
        math::Vector3 up{0, 1, 0};
        math::Matrix4 lightView = math::Matrix4::LookAt(lightPos, sceneCenter, up);
        m_csmLightMatrices[c] = lightProj * lightView;

        // Texel snapping
        const float mapRes = float(kCSMResolution);
        math::Vector4 origin = m_csmLightMatrices[c] * math::Vector4{0, 0, 0, 1};
        float invW = 1.0f / origin.w;
        float ox = (origin.x * invW * 0.5f + 0.5f) * mapRes;
        float oy = (origin.y * invW * 0.5f + 0.5f) * mapRes;
        ox = (round(ox) - ox) / mapRes * 2.0f;
        oy = (round(oy) - oy) / mapRes * 2.0f;
        math::Matrix4 snap = math::Matrix4::Identity();
        snap.m[3][0] = ox; snap.m[3][1] = oy;
        m_csmLightMatrices[c] = snap * m_csmLightMatrices[c];

        // Render into atlas sub-region
        glViewport(c * kCSMResolution, 0, kCSMResolution, kCSMResolution);
        m_csmDepthShader.Bind();
        m_csmDepthShader.SetMatrix4("u_LightSpaceMatrix", m_csmLightMatrices[c]);
        for (auto& batch : queue.GetOpaqueBatches())
        {
            if (!batch.vertexArray) continue;
            batch.vertexArray->Bind();
            for (auto& inst : batch.instances)
            {
                m_csmDepthShader.SetMatrix4("u_Model", inst.modelMatrix);
                batch.vertexArray->Draw();
            }
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    m_csmFBO.Unbind();
    core::Window& w = core::Window::instance();
    glViewport(0, 0, w.GetWidth(), w.GetHeight());
    SetCSMData();
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
        glDisable(GL_BLEND);
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

    if (mat.lightingModel == LightingModel::PBR)
    {
        // -- PBR material uniforms  (see shader/pbr.frag for slot convention)
        shader.SetInt("u_PBR_combinedMR", mat.pbrCombinedMetallicRoughness ? 1 : 0);
        shader.SetFloat("u_PBR_metallicFactor",  mat.metallicFactor);
        shader.SetFloat("u_PBR_roughnessFactor", mat.roughnessFactor);
        shader.SetFloat("u_PBR_alphaFactor",     mat.alphaFactor);
        shader.SetFloat("u_PBR_alphaCutoff",    mat.alphaCutoff);
        shader.SetFloat("u_PBR_normalScale",    mat.normalScale);
        shader.SetFloat("u_PBR_aoStrength",     mat.aoStrength);

        if (mat.albedo)
        {
            shader.SetInt("u_PBR_albedo", mat.albedo->GetSlot());
            mat.albedo->Bind();
        }
        if (mat.metallic)
        {
            shader.SetInt("u_PBR_metallic", mat.metallic->GetSlot());
            mat.metallic->Bind();
        }
        if (mat.roughness)
        {
            shader.SetInt("u_PBR_roughness", mat.roughness->GetSlot());
            mat.roughness->Bind();
        }
        if (mat.normalMap)
        {
            shader.SetInt("u_PBR_normal", mat.normalMap->GetSlot());
            mat.normalMap->Bind();
        }
        if (mat.ao)
        {
            shader.SetInt("u_PBR_ao", mat.ao->GetSlot());
            mat.ao->Bind();
        }
        if (mat.emissive)
        {
            shader.SetInt("u_PBR_emissive", mat.emissive->GetSlot());
            shader.SetVector3("u_PBR_emissiveFactor", mat.emissiveFactor);
            mat.emissive->Bind();
        }
        else
        {
            // Reset factor to zero so emissive sampler (which may still point
            // to a texture unit from a previous batch) contributes nothing.
            shader.SetVector3("u_PBR_emissiveFactor", math::Vector3{});
        }
    }
    else  // LightingModel::BlinnPhong
    {
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
    }

    if (batch.instances.size() > 1)
    {
        // Instanced path
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
    if (!m_uboReady) {
        m_frameUBO.Create(sizeof(FrameUniforms), 0);  // binding point 0
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
    shader.SetInt("u_ShadowsEnabled", m_csmShadowTex ? 1 : 0);
    if (m_csmShadowTex)
    {
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, m_csmShadowTex);
        shader.SetInt("u_ShadowMap", 6);
        shader.SetFloat("u_CSM_TexelSize", 1.0f / float(kCSMResolution));
        shader.SetFloat("u_CSM_CascadeCount", float(kCSMCascades));
        for (int c = 0; c < kCSMCascades; ++c)
        {
            char name[32];
            snprintf(name, 32, "u_CSM_LightMatrices[%d]", c);
            shader.SetMatrix4(name, m_csmLightMatrices[c]);
            snprintf(name, 32, "u_CSM_Splits[%d]", c);
            shader.SetFloat(name, m_csmSplitDepths[c]);
        }
    }
}

}
}
