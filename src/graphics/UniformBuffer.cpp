#include <sgkit/graphics/UniformBuffer.h>

#include <sgkit/framework/DebugOut.h>
#include <glad/glad.h>

namespace sgkit {
namespace graphics {

UniformBuffer::~UniformBuffer()
{
    Destroy();
}

UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    : m_handle(other.m_handle), m_binding(other.m_binding), m_size(other.m_size)
{
    other.m_handle = 0;
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        m_handle  = other.m_handle;
        m_binding = other.m_binding;
        m_size    = other.m_size;
        other.m_handle = 0;
    }
    return *this;
}

bool UniformBuffer::Create(size_t size, uint32_t bindingPoint)
{
    Destroy();
    m_size    = size;
    m_binding = bindingPoint;

    glGenBuffers(1, &m_handle);
    glBindBuffer(GL_UNIFORM_BUFFER, m_handle);
    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(size), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, m_binding, m_handle);

    SGK_LOG_INFO("UBO", "Created (handle=%u, binding=%u, size=%zu)", m_handle, m_binding, m_size);
    return m_handle != 0;
}

void UniformBuffer::Destroy()
{
    if (m_handle)
    {
        SGK_LOG_INFO("UBO", "Destroyed (handle=%u)", m_handle);
        glDeleteBuffers(1, &m_handle);
        m_handle = 0;
    }
}

bool UniformBuffer::IsValid() const
{
    return m_handle != 0;
}

void UniformBuffer::Bind() const
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_handle);
}

void UniformBuffer::Unbind() const
{
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::Upload(const void* data, size_t size, size_t offset) const
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_handle);
    glBufferSubData(GL_UNIFORM_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

uint32_t UniformBuffer::GetHandle() const
{
    return m_handle;
}

}
}
