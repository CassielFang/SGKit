#pragma once

#include <sgkit/graphics/Shader.h>
#include <sgkit/scene/Material.h>
#include <sgkit/graphics/VertexArray.h>
#include <sgkit/math/Matrix4.h>
#include <sgkit/math/Vector3.h>

#include <vector>
#include <memory>

namespace sgkit {
namespace scene {

class Mesh;

struct RenderInstance
{
    math::Matrix4 modelMatrix;
};

struct RenderBatch
{
    std::shared_ptr<graphics::Shader>      shader;
    std::shared_ptr<Material>              material;
    std::shared_ptr<graphics::VertexArray> vertexArray;
    std::vector<RenderInstance>            instances;
};

class RenderQueue
{
public:
    void Clear();

    void Submit(std::shared_ptr<Mesh> mesh, const math::Matrix4& worldMatrix);
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
