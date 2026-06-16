#include <sgkit/graphics/Mesh.h>

namespace sgkit {
namespace graphics {

void Mesh::Render() const
{
    if (!vertexArray || !material) return;
    material->Apply();
    vertexArray->Draw();
}

} // namespace graphics
} // namespace sgkit
