#include <sgkit/graphics/VertexBuffer.h>

namespace sgkit {
namespace graphics {

VertexBuffer::VertexBuffer() = default;

VertexBuffer::~VertexBuffer()
{
    Destroy();
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : m_handle(other.m_handle), m_size(other.m_size)
{
    other.m_handle = 0;
    other.m_size   = 0;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        m_handle = other.m_handle;
        m_size   = other.m_size;
        other.m_handle = 0;
        other.m_size   = 0;
    }
    return *this;
}

bool VertexBuffer::Create(const void* data, size_t sizeInBytes, uint32_t usage)
{
    Destroy();
    glGenBuffers(1, &m_handle);
    glBindBuffer(GL_ARRAY_BUFFER, m_handle);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeInBytes), data, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_size = sizeInBytes;
    return m_handle != 0;
}

void VertexBuffer::Destroy()
{
    if (m_handle)
    {
        glDeleteBuffers(1, &m_handle);
        m_handle = 0;
        m_size   = 0;
    }
}

void VertexBuffer::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_handle);
}

void VertexBuffer::Unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetData(const void* data, size_t sizeInBytes, size_t offset) const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_handle);
    glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(offset),
                    static_cast<GLsizeiptr>(sizeInBytes), data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace graphics
} // namespace sgkit
