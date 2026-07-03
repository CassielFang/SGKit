#pragma once

#include <cstdint>

namespace sgkit {
namespace graphics {

enum class Usage
{
    Static_Draw,
    Dynamic_Draw,
    Stream_Draw
};

class VertexBuffer
{
public:
    VertexBuffer();
    ~VertexBuffer();

    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    bool Create(const void* data, size_t sizeInBytes, Usage usage = Usage::Static_Draw);
    void Destroy();

    void Bind() const;
    void Unbind() const;

    void SetData(const void* data, size_t sizeInBytes, size_t offset = 0) const;

    uint32_t GetHandle() const;
    size_t   GetSize() const;
    bool     IsValid() const;

private:
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    uint32_t m_handle;
    size_t   m_size;

    static uint32_t g_currentHandle;
};

}
}
