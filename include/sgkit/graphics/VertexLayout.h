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
    int         divisor     = 0;       // 0 = per-vertex, 1 = per-instance
};

class VertexLayout
{
public:
    VertexLayout() = default;

    VertexLayout& Push(int location, int count, AttribType type, size_t elementSize, bool normalized = false);
    VertexLayout& PushFloat(int location, int count, bool normalized = false);
    VertexLayout& PushUInt(int location, int count, bool normalized = false);

    // Instanced mat4: 4×vec4 at startLocation..startLocation+3, divisor=1
    VertexLayout& PushMat4Instanced(int startLocation);

    size_t GetStride() const;
    const std::vector<VertexAttribute>& GetAttributes() const;

private:
    std::vector<VertexAttribute> m_attributes;
    size_t m_stride = 0;
};

}
}
