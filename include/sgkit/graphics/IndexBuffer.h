#pragma once

#include <glad/glad.h>

#include <cstddef>
#include <cstdint>

namespace sgkit {
namespace graphics {

class IndexBuffer
{
public:
    IndexBuffer();
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    bool Create(const uint32_t* data, size_t count, uint32_t usage = GL_STATIC_DRAW);
    void Destroy();

    void Bind() const;
    void Unbind() const;

    uint32_t GetHandle() const { return m_handle; }
    size_t   GetCount() const  { return m_count; }
    bool     IsValid() const   { return m_handle != 0; }

private:
    uint32_t m_handle = 0;
    size_t   m_count  = 0;
};

} // namespace graphics
} // namespace sgkit
