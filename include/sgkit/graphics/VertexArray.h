#pragma once

#include <memory>

#include <sgkit/graphics/VertexBuffer.h>
#include <sgkit/graphics/IndexBuffer.h>
#include <sgkit/graphics/VertexLayout.h>

namespace sgkit {
namespace graphics {

enum class DrawMode
{
    Points, Lines, LineLoop, LineStrip,
    Triangles, TriangleStrip, TriangleFan
};

class VertexArray
{
public:
    VertexArray();
    ~VertexArray();

    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    bool Create();
    void Destroy();

    void Bind() const;
    void Unbind() const;

    void AddVertexBuffer(std::shared_ptr<VertexBuffer> vb, const VertexLayout& layout);
    void SetIndexBuffer(std::shared_ptr<IndexBuffer> ib);

    void Draw(DrawMode mode = DrawMode::Triangles) const;

    uint32_t GetHandle() const;
    bool     IsValid() const;

private:
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    uint32_t m_handle;
    std::shared_ptr<VertexBuffer> m_vertexBuffer;
    std::shared_ptr<IndexBuffer>  m_indexBuffer;

    static uint32_t g_currentHandle;
};

}
}
