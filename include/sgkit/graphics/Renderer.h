#pragma once

#include <sgkit/graphics/VertexArray.h>
#include <sgkit/graphics/Mesh.h>
#include <sgkit/math/Vector4.h>

namespace sgkit {
namespace graphics {

class Renderer
{
public:
    Renderer() = default;

    void SetClearColor(const math::Vector4& color);
    void Clear();

    void SetViewport(int x, int y, int width, int height);

    void Draw(const Mesh& mesh);
    void Draw(const VertexArray& va);

    void SetWireframe(bool enabled);
    void SetDepthTest(bool enabled);
    void SetBlend(bool enabled);
    void SetCullFace(bool enabled);

private:
    math::Vector4 m_clearColor{0.1f, 0.1f, 0.15f, 1.0f};
};

} // namespace graphics
} // namespace sgkit
