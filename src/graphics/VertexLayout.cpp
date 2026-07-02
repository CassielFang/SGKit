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

}
}
