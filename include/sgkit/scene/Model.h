#pragma once

#include <sgkit/scene/Entity.h>
#include <sgkit/graphics/Shader.h>

#include <memory>
#include <vector>

namespace sgkit {
namespace scene {

// Load a 3D model file via assimp.
// Supported formats: OBJ, FBX, GLB, glTF, DAE, 3DS, PLY, STL, Blend, etc.
class Model
{
public:
    struct Result
    {
        Entity              root;      // Transform only - move/scale/hide/destroy the whole model
        std::vector<Entity> entities;  // one Entity per sub-mesh (Transform + MeshRenderer), children of root
    };

    static Result Load(const std::string& filePath,
                       std::shared_ptr<graphics::Shader> shader);
};

}
}
