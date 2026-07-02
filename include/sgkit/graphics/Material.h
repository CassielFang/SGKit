#pragma once

#include <sgkit/graphics/Shader.h>
#include <sgkit/graphics/Texture.h>
#include <sgkit/math/Vector3.h>

namespace sgkit {
namespace graphics {

class Material
{
public:
    std::shared_ptr<Shader>  shader;
    std::shared_ptr<Texture> diffuse;
    math::Vector3 specular{ 0.5f, 0.5f, 0.5f };
    float shininess = 32.0f;
};

}
}
