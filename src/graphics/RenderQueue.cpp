#include <sgkit/graphics/RenderQueue.h>
#include <sgkit/graphics/Mesh.h>

#include <algorithm>
#include <tuple>

namespace sgkit {
namespace graphics {

void RenderQueue::Clear()
{
    m_opaqueBatches.clear();
    m_transparentBatches.clear();
}

void RenderQueue::Submit(std::shared_ptr<Mesh> mesh, const math::Matrix4& worldMatrix)
{
    if (!mesh || !mesh->material || !mesh->material->shader || !mesh->vertexArray)
        return;

    Shader*      shader = mesh->material->shader.get();
    Material*    mat    = mesh->material.get();
    VertexArray* vao    = mesh->vertexArray.get();

    // Transparent materials go to the transparent list, opaque to opaque.
    bool isTransparent = (mat->blendMode != BlendMode::Opaque);
    auto& batches = isTransparent ? m_transparentBatches : m_opaqueBatches;

    // Linear search - number of unique (shader, material, vao) combos is
    // typically very small (< 100), so this is fast enough.
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

    // New batch for a previously unseen combination.
    RenderBatch batch;
    batch.shader      = mesh->material->shader;
    batch.material    = mesh->material;
    batch.vertexArray = mesh->vertexArray;
    batch.instances.push_back({worldMatrix});
    batches.push_back(std::move(batch));
}

void RenderQueue::Sort(const math::Vector3& cameraPos)
{
    // Opaque: sort by (Shader*, Material*, VAO*) to minimise state switches.
    std::sort(m_opaqueBatches.begin(), m_opaqueBatches.end(),
        [](const RenderBatch& a, const RenderBatch& b)
        {
            auto keyA = std::make_tuple(a.shader.get(), a.material.get(), a.vertexArray.get());
            auto keyB = std::make_tuple(b.shader.get(), b.material.get(), b.vertexArray.get());
            return keyA < keyB;
        });

    // Transparent: sort back-to-front for correct alpha blending.
    std::sort(m_transparentBatches.begin(), m_transparentBatches.end(),
        [&cameraPos](const RenderBatch& a, const RenderBatch& b)
        {
            // Use first instance depth as proxy for the batch.
            auto depthA = [&](const RenderBatch& rb) -> float
            {
                if (rb.instances.empty()) return 0.0f;
                const auto& m = rb.instances[0].modelMatrix.m;
                math::Vector3 pos(m[3][0], m[3][1], m[3][2]);
                return (pos - cameraPos).LengthSquared();
            };
            return depthA(a) > depthA(b);  // far -> near
        });
}

}
}
