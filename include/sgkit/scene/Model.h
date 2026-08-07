#pragma once

#include <sgkit/scene/Entity.h>
#include <sgkit/graphics/Shader.h>

#include <memory>
#include <vector>

namespace sgkit {
namespace scene {

// Load a 3D model file via assimp.
// Supported formats: OBJ, FBX, GLB, glTF, DAE, 3DS, PLY, STL, Blend, etc.
//
// Each sub-mesh auto-detects its lighting model from the source asset:
//   - PBR (glTF metallic-roughness)  ->  pbrShader
//   - Blinn-Phong (legacy specular)  ->  blinnPhongShader
//
// If only one shader is provided, it is used for ALL sub-meshes regardless of type.
class Model
{
public:
    struct Result
    {
        Entity              root;      // Transform only - move/scale/hide/destroy the whole model
        std::vector<Entity> entities;  // one Entity per sub-mesh (Transform + MeshRenderer), children of root
    };

    // Full API: assign shaders per lighting model
    static Result Load(const std::string& filePath,
                       std::shared_ptr<graphics::Shader> blinnPhongShader,
                       std::shared_ptr<graphics::Shader> pbrShader);

    // Convenience: same shader for both lighting models (backward compatible)
    static Result Load(const std::string& filePath,
                       std::shared_ptr<graphics::Shader> shader)
    {
        return Load(filePath, shader, shader);
    }
};

}
}
