#include <sgkit/graphics/IndexBuffer.h>

#include <sgkit/framework/DebugOut.h>
#include <glad/glad.h>

namespace sgkit {
namespace graphics {

IndexBuffer::IndexBuffer()
    : m_handle(0)
    , m_count(0) {}

IndexBuffer::~IndexBuffer()
{
    Destroy();
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : m_handle(other.m_handle), m_count(other.m_count)
{
    other.m_handle = 0;
    other.m_count  = 0;
    SGK_LOG_INFO("IBO", "Moved (handle=%u, count=%zu)", m_handle, m_count);
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        m_handle = other.m_handle;
        m_count  = other.m_count;
        other.m_handle = 0;
        other.m_count  = 0;
        SGK_LOG_INFO("IBO", "Move-assigned (handle=%u, count=%zu)", m_handle, m_count);
    }
    return *this;
}

bool IndexBuffer::Create(const uint32_t* data, size_t count, Usage usage)
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
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_handle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(count * sizeof(uint32_t)), data, usage_gl);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    m_count = count;
    SGK_LOG_INFO("IBO", "Created (handle=%u, count=%zu)", m_handle, m_count);
    return m_handle != 0;
}

void IndexBuffer::Destroy()
{
    if (m_handle)
    {
        SGK_LOG_INFO("IBO", "Destroyed (handle=%u, count=%zu)", m_handle, m_count);
        glDeleteBuffers(1, &m_handle);
        m_handle = 0;
        m_count  = 0;
    }
}

void IndexBuffer::Bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_handle);
}

void IndexBuffer::Unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

uint32_t IndexBuffer::GetHandle() const
{
    return m_handle;
}

size_t IndexBuffer::GetCount() const
{
    return m_count;
}

bool IndexBuffer::IsValid() const
{
    return m_handle != 0;
}

}
}
