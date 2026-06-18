#include <sgkit/graphics/IndexBuffer.h>

#include <glad/glad.h>

namespace sgkit {
namespace graphics {

IndexBuffer::IndexBuffer() = default;

IndexBuffer::~IndexBuffer()
{
    Destroy();
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : m_handle(other.m_handle), m_count(other.m_count)
{
    other.m_handle = 0;
    other.m_count  = 0;
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
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    m_count = count;
    return m_handle != 0;
}

void IndexBuffer::Destroy()
{
    if (m_handle)
    {
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

} // namespace graphics
} // namespace sgkit
