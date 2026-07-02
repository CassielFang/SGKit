#include <sgkit/graphics/VertexArray.h>

#include <glad/glad.h>

#include <cstdio>

namespace sgkit {
namespace graphics {

VertexArray::VertexArray() = default;

VertexArray::~VertexArray()
{
    Destroy();
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_handle(other.m_handle), m_vertexBuffer(std::move(other.m_vertexBuffer)),
      m_indexBuffer(std::move(other.m_indexBuffer))
{
    other.m_handle = 0;
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
    }
    return *this;
}

bool VertexArray::Create()
{
    Destroy();
    glGenVertexArrays(1, &m_handle);
    return m_handle != 0;
}

void VertexArray::Destroy()
{
    if (m_handle)
    {
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

void VertexArray::AddVertexBuffer(std::shared_ptr<VertexBuffer> vb, const VertexLayout& layout)
{
    if (!m_handle || !vb) return;

    Bind();
    vb->Bind();

    for (const auto& attr : layout.GetAttributes())
    {
        glEnableVertexAttribArray(attr.location);
        if (attr.type == AttribType::Float)
        {
            glVertexAttribPointer(attr.location, attr.count, GL_FLOAT,
                                  attr.normalized ? GL_TRUE : GL_FALSE,
                                  static_cast<GLsizei>(layout.GetStride()),
                                  reinterpret_cast<void*>(attr.offset));
        }
        else
        {
            GLenum type_gl = 0;
            switch (attr.type)
            {
            case AttribType::Byte: type_gl = GL_BYTE; break;
            case AttribType::UnsignedByte: type_gl = GL_UNSIGNED_BYTE; break;
            case AttribType::Short: type_gl = GL_SHORT; break;
            case AttribType::UnsignedShort: type_gl = GL_UNSIGNED_SHORT; break;
            case AttribType::Int: type_gl = GL_INT; break;
            case AttribType::UnsignedInt: type_gl = GL_UNSIGNED_INT; break;
            default: type_gl = GL_UNSIGNED_INT;
            }
            glVertexAttribIPointer(attr.location, attr.count, type_gl,
                                   static_cast<GLsizei>(layout.GetStride()),
                                   reinterpret_cast<void*>(attr.offset));
        }
    }

    m_vertexBuffer = vb;
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
    if (!m_handle || !m_vertexBuffer) return;

    GLenum mode_gl = 0;
    switch (mode)
    {
    case DrawMode::Points: mode_gl = GL_POINTS; break;
    case DrawMode::Lines: mode_gl = GL_LINES; break;
    case DrawMode::LineLoop: mode_gl = GL_LINE_LOOP; break;
    case DrawMode::LineStrip: mode_gl = GL_LINE_STRIP; break;
    case DrawMode::Triangles: mode_gl = GL_TRIANGLES; break;
    case DrawMode::TriangleStrip: mode_gl = GL_TRIANGLE_STRIP; break;
    case DrawMode::TriangleFan: mode_gl = GL_TRIANGLE_FAN; break;
    default: mode_gl = GL_TRIANGLES;
    }

    Bind();
    if (m_indexBuffer)
    {
        glDrawElements(mode_gl, static_cast<GLsizei>(m_indexBuffer->GetCount()),
                       GL_UNSIGNED_INT, nullptr);
    }
    else
    {
        glDrawArrays(mode_gl, 0, static_cast<GLsizei>(m_vertexBuffer->GetSize() / sizeof(float) / 3));
    }
    Unbind();
}

}
}
