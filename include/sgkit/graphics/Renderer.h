#pragma once

#include <sgkit/graphics/VertexArray.h>
#include <sgkit/graphics/RenderQueue.h>
#include <sgkit/math/Vector4.h>
#include <sgkit/math/Matrix4.h>

#include <vector>

namespace sgkit {
namespace graphics {

// GPU-ready light data for multi-light support.
struct LightData
{
    math::Vector3 position{0.0f, 0.0f, 0.0f};
    math::Vector3 ambient{0.2f, 0.2f, 0.2f};
    math::Vector3 diffuse{0.5f, 0.5f, 0.5f};
    math::Vector3 specular{1.0f, 1.0f, 1.0f};
};

class Renderer
{
public:
    static void Create();
    static void Destroy();
    static Renderer& instance();

    void SetClearColor(const math::Vector4& color);
    void Clear();

    // Low-level draw - for users who bypass Scene and render directly with graphics.
    void Draw(const VertexArray& va);

    void SetViewport(int x, int y, int width, int height);
    void SetWireframe(bool enabled);
    void SetDepthTest(bool enabled);
    void SetBlend(bool enabled);
    void SetCullFace(bool enabled);

    // -- Frame-level data
    void SetViewProjection(const math::Matrix4& vp);
    void SetCameraPosition(const math::Vector3& pos);
    void SetAmbientLight(const math::Vector3& color);
    void SetLights(const std::vector<LightData>& lights);

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
    math::Vector3 m_ambientLight{0.1f, 0.1f, 0.15f};
    std::vector<LightData> m_lights;

    void ExecuteBatch(const RenderBatch& batch);
    void ApplyBatchState(const RenderBatch& batch);
    void SetFrameUniforms(Shader& shader);
};

}
}
