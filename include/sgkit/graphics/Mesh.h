#pragma once

#include <memory>
#include <sgkit/graphics/VertexArray.h>
#include <sgkit/graphics/Material.h>

namespace sgkit {
namespace graphics {

struct Mesh
{
    std::shared_ptr<VertexArray> vertexArray;
    std::shared_ptr<Material>    material;

    void Render() const;
};

} // namespace graphics
} // namespace sgkit
