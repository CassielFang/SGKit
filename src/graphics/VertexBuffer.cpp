#include <sgkit/graphics/VertexBuffer.h>

#include <sgkit/framework/DebugOut.h>
#include <glad/glad.h>

namespace sgkit {
namespace graphics {

VertexBuffer::VertexBuffer()
    : m_handle(0)
    , m_size(0) {}

VertexBuffer::~VertexBuffer()
{
    Destroy();
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : m_handle(other.m_handle), m_size(other.m_size)
{
    other.m_handle = 0;
    other.m_size   = 0;
    SGK_LOG_INFO("VBO", "Moved (handle=%u, size=%zu)", m_handle, m_size);
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
        SGK_LOG_INFO("VBO", "Move-assigned (handle=%u, size=%zu)", m_handle, m_size);
    }
    return *this;
}

bool VertexBuffer::Create(const void* data, size_t sizeInBytes, Usage usage)
{
    Destroy();
    GLenum usage_gl = GL_STATIC_DRAW;
    if (usage == Usage::Dynamic_Draw)
    {
        usage_gl = GL_DYNAMIC_DRAW;
    }
    else if (usage == Usage::Stream_Draw)
    {
        usage_gl = GL_STREAM_DRAW;
    }
    glGenBuffers(1, &m_handle);
    glBindBuffer(GL_ARRAY_BUFFER, m_handle);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeInBytes), data, usage_gl);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_size = sizeInBytes;
    SGK_LOG_INFO("VBO", "Created (handle=%u, size=%zu bytes)", m_handle, m_size);
    return m_handle != 0;
}

void VertexBuffer::Destroy()
{
    if (m_handle)
    {
        SGK_LOG_INFO("VBO", "Destroyed (handle=%u, size=%zu)", m_handle, m_size);
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
    glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(sizeInBytes), data);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
}

uint32_t VertexBuffer::GetHandle() const
{
    return m_handle;
}

size_t VertexBuffer::GetSize() const
{
    return m_size;
}

bool VertexBuffer::IsValid() const
{
    return m_handle != 0;
}

}
}
