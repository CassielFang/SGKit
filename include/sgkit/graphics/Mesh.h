#pragma once

#include <sgkit/graphics/VertexArray.h>
#include <sgkit/graphics/Material.h>
#include <sgkit/graphics/RenderContext.h>
#include <sgkit/math/Matrix4.h>

namespace sgkit {
namespace graphics {

class Mesh
{
public:
    std::shared_ptr<VertexArray> vertexArray;
    std::shared_ptr<Material>    material;

    /**
    * Full-featured draw with per-instance transform and per-frame context.
    * Handles global uniforms (u_ViewProjection, u_cameraPos, u_Light.*),
    * instance uniform (u_Model), and delegates material uniforms to
    * Material::Apply().
    */
    void Render(const math::Matrix4& modelMatrix, const RenderContext& ctx) const;

    // Simple draw with identity model and empty context.
    // Provided as a fallback for quick-and-dirty rendering.
    void Render() const;
};

}
}
