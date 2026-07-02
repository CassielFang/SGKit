#pragma once

#include <sgkit/graphics/VertexArray.h>
#include <sgkit/graphics/Mesh.h>
#include <sgkit/math/Vector4.h>

namespace sgkit {
namespace graphics {

class Renderer
{
public:
    static void Create();
    static void Destroy();
    static Renderer& instance();

    void SetClearColor(const math::Vector4& color);
    void Clear();

    void Draw(const Mesh& mesh);
    void Draw(const Mesh& mesh, const math::Matrix4& model, const RenderContext& ctx);
    void Draw(const VertexArray& va);

    void SetViewport(int x, int y, int width, int height);
    void SetWireframe(bool enabled);
    void SetDepthTest(bool enabled);
    void SetBlend(bool enabled);
    void SetCullFace(bool enabled);

private:
    Renderer() = default;
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(const Renderer&&) = delete;
    Renderer& operator=(const Renderer&&) = delete;

    math::Vector4 m_clearColor{0.1f, 0.1f, 0.15f, 1.0f};
};

}
}
