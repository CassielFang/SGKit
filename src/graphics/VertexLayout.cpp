#include <sgkit/graphics/VertexLayout.h>

#include <glad/glad.h>

namespace sgkit {
namespace graphics {

VertexLayout& VertexLayout::Push(int location, int count, AttribType type, size_t elementSize, bool normalized)
{
    VertexAttribute attr;
    attr.location   = location;
    attr.count      = count;
    attr.type       = type;
    attr.normalized = normalized;
    attr.offset     = m_stride;

    m_attributes.push_back(attr);
    m_stride += static_cast<size_t>(count) * elementSize;
    return *this;
}

VertexLayout& VertexLayout::PushFloat(int location, int count, bool normalized)
{
    return Push(location, count, AttribType::Float, sizeof(float), normalized);
}

VertexLayout& VertexLayout::PushUInt(int location, int count, bool normalized)
{
    return Push(location, count, AttribType::UnsignedInt, sizeof(uint32_t), normalized);
}

VertexLayout& VertexLayout::PushMat4Instanced(int startLocation)
{
    // A mat4 = 4 vec4 columns, each as a separate instanced attribute
    for (int col = 0; col < 4; ++col)
    {
        VertexAttribute a;
        a.location = startLocation + col;
        a.count    = 4;
        a.type     = AttribType::Float;
        a.offset   = m_stride + col * 4 * sizeof(float);
        a.divisor  = 1;  // one per instance
        m_attributes.push_back(a);
    }
    m_stride += sizeof(float) * 16;  // 4×4 matrix
    return *this;
}

size_t VertexLayout::GetStride() const
{
    return m_stride;
}

const std::vector<VertexAttribute>& VertexLayout::GetAttributes() const
{
    return m_attributes;
}

}
}
