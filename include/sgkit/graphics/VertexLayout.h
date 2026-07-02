#pragma once

#include <string>
#include <vector>

namespace sgkit {
namespace graphics {

enum class AttribType
{
    Byte, UnsignedByte,
    Short, UnsignedShort,
    Int, UnsignedInt,
    Float
};

struct VertexAttribute
{
    std::string name;
    int         location    = 0;
    int         count       = 3;       // 1, 2, 3, or 4
    AttribType  type        = AttribType::Float;
    bool        normalized  = false;
    size_t      offset      = 0;
};

class VertexLayout
{
public:
    VertexLayout() = default;

    VertexLayout& Push(int location, int count, AttribType type, size_t elementSize, bool normalized = false);
    VertexLayout& PushFloat(int location, int count, bool normalized = false);
    VertexLayout& PushUInt(int location, int count, bool normalized = false);

    size_t GetStride() const { return m_stride; }
    const std::vector<VertexAttribute>& GetAttributes() const { return m_attributes; }

private:
    std::vector<VertexAttribute> m_attributes;
    size_t m_stride = 0;
};

}
}
