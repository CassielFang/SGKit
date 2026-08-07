#include <sgkit/graphics/VertexArray.h>

#include <sgkit/framework/DebugOut.h>
#include <glad/glad.h>

namespace sgkit {
namespace graphics {

VertexArray::VertexArray() : m_handle(0) {}

VertexArray::~VertexArray()
{
    Destroy();
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_handle(other.m_handle), m_vertexBuffer(std::move(other.m_vertexBuffer)),
      m_indexBuffer(std::move(other.m_indexBuffer))
{
    other.m_handle = 0;
    SGK_LOG_INFO("VAO", "Moved (handle=%u)", m_handle);
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        m_handle       = other.m_handle;
        m_vertexBuffer = std::move(other.m_vertexBuffer);
        m_indexBuffer  = std::move(other.m_indexBuffer);
        other.m_handle = 0;
        SGK_LOG_INFO("VAO", "Move-assigned (handle=%u)", m_handle);
    }
    return *this;
}

bool VertexArray::Create()
{
    Destroy();
    glGenVertexArrays(1, &m_handle);
    SGK_LOG_INFO("VAO", "Created (handle=%u)", m_handle);
    return m_handle != 0;
}

void VertexArray::Destroy()
{
    if (m_handle)
    {
        SGK_LOG_INFO("VAO", "Destroyed (handle=%u)", m_handle);
        glDeleteVertexArrays(1, &m_handle);
        m_handle = 0;
    }
    m_vertexBuffer.reset();
    m_indexBuffer.reset();
}

void VertexArray::Bind() const
{
    glBindVertexArray(m_handle);
}

void VertexArray::Unbind() const
{
    glBindVertexArray(0);
}

static void SetupAttrib(const VertexAttribute& attr, GLsizei stride)
{
    glEnableVertexAttribArray(attr.location);
    if (attr.type == AttribType::Float)
        glVertexAttribPointer(attr.location, attr.count, GL_FLOAT,
                              attr.normalized ? GL_TRUE : GL_FALSE,
                              stride, reinterpret_cast<void*>(attr.offset));
    else
    {
        GLenum t = GL_UNSIGNED_INT;
        switch (attr.type)
        {
        case AttribType::Byte:          t = GL_BYTE;           break;
        case AttribType::UnsignedByte:  t = GL_UNSIGNED_BYTE;  break;
        case AttribType::Short:         t = GL_SHORT;          break;
        case AttribType::UnsignedShort: t = GL_UNSIGNED_SHORT; break;
        case AttribType::Int:           t = GL_INT;            break;
        default: break;
        }
        glVertexAttribIPointer(attr.location, attr.count, t, stride,
                               reinterpret_cast<void*>(attr.offset));
    }
    if (attr.divisor > 0)
        glVertexAttribDivisor(attr.location, attr.divisor);
}

void VertexArray::SetVertexBuffer(std::shared_ptr<VertexBuffer> vb, const VertexLayout& layout)
{
    if (!m_handle || !vb) return;
    Bind();
    vb->Bind();
    for (const auto& attr : layout.GetAttributes())
        SetupAttrib(attr, static_cast<GLsizei>(layout.GetStride()));
    m_vertexBuffer = vb;
    Unbind();
}

void VertexArray::SetInstanceBuffer(std::shared_ptr<VertexBuffer> ib, const VertexLayout& layout)
{
    if (!m_handle || !ib) return;
    Bind();
    ib->Bind();
    for (const auto& attr : layout.GetAttributes())
        SetupAttrib(attr, static_cast<GLsizei>(layout.GetStride()));
    Unbind();
}

void VertexArray::SetIndexBuffer(std::shared_ptr<IndexBuffer> ib)
{
    if (!m_handle) return;
    Bind();
    ib->Bind();
    m_indexBuffer = ib;
    Unbind();
}

void VertexArray::Draw(DrawMode mode) const
{
    DrawInstanced(1, mode);
}

void VertexArray::DrawInstanced(uint32_t instanceCount, DrawMode mode) const
{
    if (!m_handle || !m_vertexBuffer) return;

    GLenum mode_gl = GL_TRIANGLES;
    switch (mode)
    {
    case DrawMode::Points:        mode_gl = GL_POINTS;         break;
    case DrawMode::Lines:         mode_gl = GL_LINES;          break;
    case DrawMode::LineLoop:      mode_gl = GL_LINE_LOOP;      break;
    case DrawMode::LineStrip:     mode_gl = GL_LINE_STRIP;     break;
    case DrawMode::Triangles:     mode_gl = GL_TRIANGLES;      break;
    case DrawMode::TriangleStrip: mode_gl = GL_TRIANGLE_STRIP; break;
    case DrawMode::TriangleFan:   mode_gl = GL_TRIANGLE_FAN;   break;
    }

    Bind();
    if (m_indexBuffer)
        glDrawElementsInstanced(mode_gl, static_cast<GLsizei>(m_indexBuffer->GetCount()),
                                GL_UNSIGNED_INT, nullptr, instanceCount);
    else
        glDrawArraysInstanced(mode_gl, 0,
            static_cast<GLsizei>(m_vertexBuffer->GetSize() / sizeof(float) / 3),
            instanceCount);
}

uint32_t VertexArray::GetHandle() const
{
    return m_handle;
}

bool VertexArray::IsValid() const
{
    return m_handle != 0;
}

}
}
