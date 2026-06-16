#pragma once

#include <glad/glad.h>

#include <cstdint>
#include <memory>
#include <vector>

#include <sgkit/graphics/VertexBuffer.h>
#include <sgkit/graphics/IndexBuffer.h>
#include <sgkit/graphics/VertexLayout.h>

namespace sgkit {
namespace graphics {

class VertexArray
{
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    bool Create();
    void Destroy();

    void Bind() const;
    void Unbind() const;

    void AddVertexBuffer(std::shared_ptr<VertexBuffer> vb, const VertexLayout& layout);
    void SetIndexBuffer(std::shared_ptr<IndexBuffer> ib);

    void Draw(uint32_t mode = GL_TRIANGLES) const;

    uint32_t GetHandle() const { return m_handle; }
    bool     IsValid() const   { return m_handle != 0; }

private:
    uint32_t m_handle = 0;
    std::shared_ptr<VertexBuffer> m_vertexBuffer;
    std::shared_ptr<IndexBuffer>  m_indexBuffer;
};

} // namespace graphics
} // namespace sgkit
