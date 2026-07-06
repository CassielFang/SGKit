#include <sgkit/scene/RenderQueue.h>
#include <sgkit/scene/Mesh.h>

#include <algorithm>
#include <tuple>

namespace sgkit {
namespace scene {

void RenderQueue::Clear()
{
    m_opaqueBatches.clear();
    m_transparentBatches.clear();
}

void RenderQueue::Submit(std::shared_ptr<Mesh> mesh, const math::Matrix4& worldMatrix)
{
    if (!mesh || !mesh->material || !mesh->material->shader || !mesh->vertexArray)
        return;

    graphics::Shader*       shader = mesh->material->shader.get();
    Material*               mat    = mesh->material.get();
    graphics::VertexArray*  vao    = mesh->vertexArray.get();

    bool isTransparent = (mat->blendMode != BlendMode::Opaque);
    auto& batches = isTransparent ? m_transparentBatches : m_opaqueBatches;

    for (auto& batch : batches)
    {
        if (batch.shader.get() == shader &&
            batch.material.get() == mat &&
            batch.vertexArray.get() == vao)
        {
            batch.instances.push_back({worldMatrix});
            return;
        }
    }

    RenderBatch batch;
    batch.shader      = mesh->material->shader;
    batch.material    = mesh->material;
    batch.vertexArray = mesh->vertexArray;
    batch.instances.push_back({worldMatrix});
    batches.push_back(std::move(batch));
}

void RenderQueue::Sort(const math::Vector3& cameraPos)
{
    std::sort(m_opaqueBatches.begin(), m_opaqueBatches.end(),
        [](const RenderBatch& a, const RenderBatch& b)
        {
            auto keyA = std::make_tuple(a.shader.get(), a.material.get(), a.vertexArray.get());
            auto keyB = std::make_tuple(b.shader.get(), b.material.get(), b.vertexArray.get());
            return keyA < keyB;
        });

    std::sort(m_transparentBatches.begin(), m_transparentBatches.end(),
        [&cameraPos](const RenderBatch& a, const RenderBatch& b)
        {
            auto depth = [&](const RenderBatch& rb) -> float
            {
                if (rb.instances.empty()) return 0.0f;
                const auto& m = rb.instances[0].modelMatrix.m;
                math::Vector3 pos(m[3][0], m[3][1], m[3][2]);
                return (pos - cameraPos).LengthSquared();
            };
            return depth(a) > depth(b);
        });
}

}
}
