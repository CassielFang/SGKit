#include <sgkit/scene/CSMShadow.h>

#include <sgkit/scene/RenderQueue.h>
#include <sgkit/core/Window.h>
#include <glad/glad.h>

namespace sgkit {
namespace scene {

void CSMShadow::LazyInit()
{
    if (m_ready) return;

    int atlasW = kResolution * kCascades;
    m_fbo.Create(atlasW, kResolution);
    m_depthShader.LoadFromFile(
        "assets/shaders/shadow_depth.vert",
        "assets/shaders/shadow_depth.frag");
    m_shadowTex = m_fbo.GetDepthTexture();
    m_ready = true;
}

void CSMShadow::RenderPass(
    const RenderQueue& queue, const math::Vector3& lightDir,
    const math::Matrix4& camView, const math::Matrix4& camProj,
    const math::Vector3& cameraPos, float cameraNear, float cameraFar,
    float aspect)
{
    LazyInit();

    m_fbo.Bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(4.0f, 4.0f);

    // -- Cascade split depths (lambda=0.9 for tighter near cascades)
    float lambda = 0.9f;
    for (int i = 0; i < kCascades; ++i)
    {
        float p = float(i + 1) / kCascades;
        float logS = cameraNear * pow(cameraFar / cameraNear, p);
        float linS = cameraNear + (cameraFar - cameraNear) * p;
        m_splitDepths[i] = logS * lambda + linS * (1.0f - lambda);
    }

    // -- Extract camera forward from view matrix
    math::Vector3 camForward = math::Vector3{
        -camView.m[2][0], -camView.m[2][1], -camView.m[2][2]
    }.Normalized();

    // -- Full camera frustum corners in world space
    math::Matrix4 invCamVP = (camProj * camView).Inverted();

    math::Vector4 ndcCorners[8] = {
        {-1.f, -1.f, -1.f, 1.f}, { 1.f, -1.f, -1.f, 1.f},
        {-1.f,  1.f, -1.f, 1.f}, { 1.f,  1.f, -1.f, 1.f},
        {-1.f, -1.f,  1.f, 1.f}, { 1.f, -1.f,  1.f, 1.f},
        {-1.f,  1.f,  1.f, 1.f}, { 1.f,  1.f,  1.f, 1.f},
    };

    math::Vector3 worldCorners[8];
    for (int i = 0; i < 8; ++i)
    {
        math::Vector4 ws = invCamVP * ndcCorners[i];
        float invW = 1.0f / ws.w;
        worldCorners[i] = {ws.x * invW, ws.y * invW, ws.z * invW};
    }

    // -- Light up vector with degeneracy fallback
    math::Vector3 lightDirN = lightDir.Normalized();
    math::Vector3 up{0.f, 1.f, 0.f};
    if (std::abs(math::Vector3::Dot(lightDirN, up)) > 0.999f)
        up = {1.f, 0.f, 0.f};

    for (int c = 0; c < kCascades; ++c)
    {
        float prevDist = (c == 0) ? cameraNear : m_splitDepths[c - 1];
        float curDist  = m_splitDepths[c];

        float nearRatio = prevDist / cameraFar;
        float farRatio  = curDist  / cameraFar;

        math::Vector3 cascadeCorners[8];
        for (int i = 0; i < 4; ++i)
        {
            const math::Vector3& n = worldCorners[i];
            const math::Vector3& f = worldCorners[i + 4];
            cascadeCorners[i]     = n + (f - n) * nearRatio;
            cascadeCorners[i + 4] = n + (f - n) * farRatio;
        }

        math::Vector3 cascadeCenter{0, 0, 0};
        for (int i = 0; i < 8; ++i) cascadeCenter = cascadeCenter + cascadeCorners[i];
        cascadeCenter = cascadeCenter / 8.0f;

        math::Vector3 lightPos = cascadeCenter - lightDirN * 100.0f;
        math::Matrix4 lightView = math::Matrix4::LookAt(lightPos, cascadeCenter, up);

        float minX = 1e10f, maxX = -1e10f;
        float minY = 1e10f, maxY = -1e10f;
        float minZ = 1e10f, maxZ = -1e10f;
        for (int i = 0; i < 8; ++i)
        {
            math::Vector4 ls = lightView * math::Vector4{cascadeCorners[i], 1.0f};
            minX = std::min(minX, ls.x); maxX = std::max(maxX, ls.x);
            minY = std::min(minY, ls.y); maxY = std::max(maxY, ls.y);
            minZ = std::min(minZ, ls.z); maxZ = std::max(maxZ, ls.z);
        }

        float zPad = 80.0f;
        float nearDist = -maxZ;
        float farDist  = -minZ;
        nearDist = std::max(0.1f, nearDist - zPad);
        farDist  = std::max(nearDist + 10.0f, farDist + zPad);

        math::Matrix4 lightProj = math::Matrix4::Orthographic(
            minX, maxX, minY, maxY, nearDist, farDist);
        m_lightMatrices[c] = lightProj * lightView;

        // Texel snapping
        const float mapRes = float(kResolution);
        math::Vector4 origin = m_lightMatrices[c] * math::Vector4{0, 0, 0, 1};
        float invW = 1.0f / origin.w;
        float ox = (origin.x * invW * 0.5f + 0.5f) * mapRes;
        float oy = (origin.y * invW * 0.5f + 0.5f) * mapRes;
        ox = (round(ox) - ox) / mapRes * 2.0f;
        oy = (round(oy) - oy) / mapRes * 2.0f;
        math::Matrix4 snap = math::Matrix4::Identity();
        snap.m[3][0] = ox; snap.m[3][1] = oy;
        m_lightMatrices[c] = snap * m_lightMatrices[c];

        // Render into atlas sub-region
        glViewport(c * kResolution, 0, kResolution, kResolution);
        m_depthShader.Bind();
        m_depthShader.SetMatrix4("u_LightSpaceMatrix", m_lightMatrices[c]);
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

    glDisable(GL_POLYGON_OFFSET_FILL);
    m_fbo.Unbind();
    core::Window& w = core::Window::instance();
    glViewport(0, 0, w.GetWidth(), w.GetHeight());
}

void CSMShadow::ApplyToShader(graphics::Shader& shader) const
{
    if (!m_shadowTex) return;

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, m_shadowTex);
    shader.SetInt("u_ShadowMap", 6);
    shader.SetFloat("u_CSM_TexelSize", 1.0f / float(kResolution));
    shader.SetFloat("u_CSM_CascadeCount", float(kCascades));
    shader.SetFloat("u_CSM_AtlasTexelX", 1.0f / float(kResolution * kCascades));

    for (int c = 0; c < kCascades; ++c)
    {
        char name[32];
        snprintf(name, 32, "u_CSM_LightMatrices[%d]", c);
        shader.SetMatrix4(name, m_lightMatrices[c]);
        snprintf(name, 32, "u_CSM_Splits[%d]", c);
        shader.SetFloat(name, m_splitDepths[c]);
    }
}

bool CSMShadow::IsReady() const { return m_shadowTex != 0; }
uint32_t CSMShadow::GetShadowTex() const { return m_shadowTex; }
const math::Matrix4* CSMShadow::LightMatrices() const { return m_lightMatrices; }
const float* CSMShadow::SplitDepths() const { return m_splitDepths; }

}
}
