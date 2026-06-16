#include <sgkit/graphics/Material.h>

namespace sgkit {
namespace graphics {

void Material::Apply() const
{
    if (!shader) return;

    shader->Bind();

    shader->SetVector3("u_Material.ambient", ambientColor);
    shader->SetVector3("u_Material.diffuse", diffuseColor);
    shader->SetVector3("u_Material.specular", specularColor);
    shader->SetFloat("u_Material.shininess", shininess);
    shader->SetFloat("u_Material.opacity", opacity);

    if (diffuseTexture)
    {
        shader->SetInt("u_DiffuseTexture", 0);
        diffuseTexture->Bind(0);
    }
}

} // namespace graphics
} // namespace sgkit
