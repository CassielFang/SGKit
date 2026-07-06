#pragma once

#include <sgkit/scene/RenderQueue.h>
#include <sgkit/graphics/VertexArray.h>
#include <sgkit/math/Vector4.h>
#include <sgkit/math/Matrix4.h>

#include <vector>

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
    void SetLights(const std::vector<LightInstance>& instances);

    // Execute a sorted render queue (two-pass: opaque -> transparent).
    void Execute(const RenderQueue& queue);

private:
    Renderer() = default;
    ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    math::Matrix4 m_viewProjection = math::Matrix4::Identity();
    math::Vector3 m_cameraPos{0.0f, 0.0f, 0.0f};
    std::vector<LightInstance> m_lights;

    void ExecuteBatch(const RenderBatch& batch);
    void ApplyBatchState(const RenderBatch& batch);
    void SetFrameUniforms(graphics::Shader& shader);
};

}
}
