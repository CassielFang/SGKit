#pragma once

#include <memory>
#include <sgkit/graphics/Shader.h>
#include <sgkit/graphics/Texture.h>
#include <sgkit/math/Vector3.h>

namespace sgkit {
namespace graphics {

struct Material
{
    std::shared_ptr<Shader>  shader;
    std::shared_ptr<Texture> diffuseTexture;

    math::Vector3 ambientColor{0.2f, 0.2f, 0.2f};
    math::Vector3 diffuseColor{0.8f, 0.8f, 0.8f};
    math::Vector3 specularColor{1.0f, 1.0f, 1.0f};
    float shininess = 32.0f;
    float opacity   = 1.0f;

    void Apply() const;
};

} // namespace graphics
} // namespace sgkit
