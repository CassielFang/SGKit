#pragma once

#include <sgkit/graphics/VertexArray.h>
#include <sgkit/scene/Material.h>

namespace sgkit {
namespace scene {

class Mesh
{
public:
    std::shared_ptr<graphics::VertexArray> vertexArray;
    std::shared_ptr<Material>              material;
};

}
}
