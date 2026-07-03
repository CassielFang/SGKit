#pragma once

#include <sgkit/graphics/Shader.h>
#include <sgkit/graphics/Material.h>
#include <sgkit/graphics/VertexArray.h>
#include <sgkit/math/Matrix4.h>
#include <sgkit/math/Vector3.h>

#include <vector>
#include <memory>

namespace sgkit {
namespace graphics {

// Forward-declare Mesh - full definition not needed for shared_ptr param.
class Mesh;

struct RenderInstance
{
    math::Matrix4 modelMatrix;
};

struct RenderBatch
{
    std::shared_ptr<Shader>      shader;
    std::shared_ptr<Material>    material;
    std::shared_ptr<VertexArray> vertexArray;
    std::vector<RenderInstance>  instances;
};

class RenderQueue
{
public:
    void Clear();

    /**
    * Submit one mesh instance.  Internally groups draws that share the
    * same (Shader*, Material*, VertexArray*) into one batch so that
    * GPU state switches are minimised during execution.
    */
    void Submit(std::shared_ptr<Mesh> mesh, const math::Matrix4& worldMatrix);

    /**
    * Sort batches.  Opaque batches are ordered by (shader, material, vao)
    * to minimise state changes; transparent batches are sorted back-to-
    * front relative to `cameraPos` for correct blending.
    */
    void Sort(const math::Vector3& cameraPos);

    const std::vector<RenderBatch>& GetOpaqueBatches()      const { return m_opaqueBatches; }
    const std::vector<RenderBatch>& GetTransparentBatches() const { return m_transparentBatches; }
    bool HasTransparentBatches() const { return !m_transparentBatches.empty(); }

private:
    std::vector<RenderBatch> m_opaqueBatches;
    std::vector<RenderBatch> m_transparentBatches;
};

}
}
