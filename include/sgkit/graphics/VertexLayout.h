#pragma once

#include <glad/glad.h>

#include <cstddef>
#include <string>
#include <vector>

namespace sgkit {
namespace graphics {

struct VertexAttribute
{
    std::string name;
    int         location    = 0;
    int         count       = 3;       // 1, 2, 3, or 4
    uint32_t    type        = GL_FLOAT;
    bool        normalized  = false;
    size_t      offset      = 0;
};

class VertexLayout
{
public:
    VertexLayout() = default;

    VertexLayout& PushFloat(int location, int count, bool normalized = false);
    VertexLayout& PushUInt(int location, int count, bool normalized = false);

    size_t GetStride() const { return m_stride; }
    const std::vector<VertexAttribute>& GetAttributes() const { return m_attributes; }

private:
    std::vector<VertexAttribute> m_attributes;
    size_t m_stride = 0;

    VertexLayout& Push(int location, int count, uint32_t type, size_t elementSize, bool normalized);
};

} // namespace graphics
} // namespace sgkit
