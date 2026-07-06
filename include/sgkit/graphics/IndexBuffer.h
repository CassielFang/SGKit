#pragma once

#include <sgkit/graphics/VertexBuffer.h>

namespace sgkit {
namespace graphics {

class IndexBuffer
{
public:
    IndexBuffer();
    ~IndexBuffer();

    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    bool Create(const uint32_t* data, size_t count, Usage usage = Usage::Static_Draw);
    void Destroy();

    void Bind() const;
    void Unbind() const;

    uint32_t GetHandle() const;
    size_t   GetCount() const;
    bool     IsValid() const;

private:
    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    uint32_t m_handle;
    size_t   m_count;
};

}
}
