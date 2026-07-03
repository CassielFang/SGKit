#pragma once

#include <sgkit/graphics/Shader.h>
#include <sgkit/graphics/Texture.h>
#include <sgkit/math/Vector3.h>

#include <memory>

namespace sgkit {
namespace graphics {

enum class BlendMode
{
    Opaque,
    AlphaBlend,
    Additive,
};

enum class CullMode
{
    Back,
    Front,
    None,
};

enum class DepthMode
{
    ReadWrite,
    ReadOnly,
    None,
};

class Material
{
public:
    std::shared_ptr<Shader>  shader;
    std::shared_ptr<Texture> diffuse;
    math::Vector3 specular{0.5f, 0.5f, 0.5f};
    float shininess = 32.0f;

    BlendMode blendMode = BlendMode::Opaque;
    CullMode  cullMode  = CullMode::Back;
    DepthMode depthMode = DepthMode::ReadWrite;
    int renderQueue = 0;
};

}
}
