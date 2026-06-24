#include <sgkit/graphics/Material.h>

namespace sgkit {
namespace graphics {

void Material::Apply() const
{
    if (shader && diffuse)
    {
        shader->Bind();
        shader->SetInt("u_Material.diffuse", 0);
        diffuse->Bind(0);
        shader->SetVector3("u_Material.specular", specular);
        shader->SetFloat("u_Material.shininess", shininess);
    }
}

} // namespace graphics
} // namespace sgkit
