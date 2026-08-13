#pragma once

#include <sgkit/scene/RenderQueue.h>
#include <sgkit/scene/LightData.h>
#include <sgkit/scene/DirectionalShadow.h>
#include <sgkit/scene/SkyboxRenderer.h>
#include <sgkit/graphics/UniformBuffer.h>
#include <sgkit/math/Vector4.h>
#include <sgkit/math/Matrix4.h>

#include <vector>
#include <string>

namespace sgkit {
namespace scene {

namespace component
{
    class Light;
}

// Self-contained light data - no pointer to external graphics resource.
class LightInstance
{
public:
    math::Vector3 worldPosition {0.0f, 0.0f, 0.0f};
    component::Light* attribute = nullptr;
};

class Renderer
{
public:
    static void Create();
    static void Destroy();
    static Renderer& instance();

    void SetClearColor(const math::Vector4& color);
    void Clear();

    void SetViewport(int x, int y, int width, int height);
    void SetWireframe(bool enabled);
    void SetDepthTest(bool enabled);
    void SetBlend(bool enabled);
    void SetCullFace(bool enabled);

    // -- Frame-level data
    void SetViewProjection(const math::Matrix4& vp);
    void SetCameraPosition(const math::Vector3& pos);
    void SetCameraForward(const math::Vector3& fwd);
    void SetLights(const std::vector<LightInstance>& instances);
    void CommitFrameData();   // upload UBO once per frame

    // Execute a sorted render queue (two-pass: opaque -> transparent).
    void Execute(const RenderQueue& queue);

    // -- Directional shadow (delegates to DirectionalShadow)
    void RenderDirectionalShadowPass(
        const RenderQueue& queue, const math::Vector3& lightDir);

    // -- Skybox (delegates to SkyboxRenderer)
    bool SetupSkybox(const std::string& hdrPath);
    void SetSkyboxMatrices(const math::Matrix4& view, const math::Matrix4& proj);
    void DestroySkybox();

private:
    Renderer() = default;
    ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    // Sub-systems
    DirectionalShadow m_dirShadow;
    SkyboxRenderer    m_skybox;
    math::Matrix4  m_skyboxView = math::Matrix4::Identity();
    math::Matrix4  m_skyboxProj = math::Matrix4::Identity();

    // Frame data
    math::Vector3 m_cameraPos     { 0.0f, 0.0f,  0.0f };
    math::Vector3 m_cameraForward { 0.0f, 0.0f, -1.0f };

    // UBO
    graphics::UniformBuffer m_frameUBO;
    FrameUniforms           m_frameData{};
    bool                    m_uboReady = false;

    void EnsureUBO();
    void UpdateFrameUBO();

    void ExecuteBatch(const RenderBatch& batch);
    void ApplyBatchState(const RenderBatch& batch);
    void SetFrameUniforms(graphics::Shader& shader);
};

}
}
