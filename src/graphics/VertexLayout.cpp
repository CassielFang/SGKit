#include <sgkit/graphics/VertexLayout.h>

namespace sgkit {
namespace graphics {

VertexLayout& VertexLayout::Push(int location, int count, uint32_t type, size_t elementSize, bool normalized)
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
    return Push(location, count, GL_FLOAT, sizeof(float), normalized);
}

VertexLayout& VertexLayout::PushUInt(int location, int count, bool normalized)
{
    return Push(location, count, GL_UNSIGNED_INT, sizeof(uint32_t), normalized);
}

} // namespace graphics
} // namespace sgkit
