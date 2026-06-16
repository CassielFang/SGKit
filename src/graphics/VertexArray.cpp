#include <sgkit/graphics/VertexArray.h>

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
        if (attr.type == GL_FLOAT)
        {
            glVertexAttribPointer(attr.location, attr.count, attr.type,
                                  attr.normalized ? GL_TRUE : GL_FALSE,
                                  static_cast<GLsizei>(layout.GetStride()),
                                  reinterpret_cast<void*>(attr.offset));
        }
        else
        {
            glVertexAttribIPointer(attr.location, attr.count, attr.type,
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

void VertexArray::Draw(uint32_t mode) const
{
    if (!m_handle || !m_vertexBuffer) return;

    Bind();
    if (m_indexBuffer)
    {
        glDrawElements(mode, static_cast<GLsizei>(m_indexBuffer->GetCount()),
                       GL_UNSIGNED_INT, nullptr);
    }
    else
    {
        glDrawArrays(mode, 0, static_cast<GLsizei>(m_vertexBuffer->GetSize() / sizeof(float) / 3));
    }
    Unbind();
}

} // namespace graphics
} // namespace sgkit
