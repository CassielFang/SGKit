#include <sgkit/graphics/Mesh.h>

namespace sgkit {
namespace graphics {

void Mesh::Render(const math::Matrix4& modelMatrix, const RenderContext& ctx) const
{
    if (!vertexArray || !material || !material->shader)
        return;

    auto& shader = material->shader;
    
    shader->Bind();
    shader->SetMatrix4("u_Model", modelMatrix);
    shader->SetMatrix4("u_ViewProjection", ctx.viewProjection);
    shader->SetVector3("u_cameraPos", ctx.cameraPos);

    if (ctx.hasLight)
    {
        shader->SetVector3("u_Light.position", ctx.light.position);
        shader->SetVector3("u_Light.ambient",  ctx.light.ambient);
        shader->SetVector3("u_Light.diffuse",  ctx.light.diffuse);
        shader->SetVector3("u_Light.specular", ctx.light.specular);
    }

    // Material uniforms.
    if (material->diffuse)
    {
        shader->SetInt("u_Material.diffuse", 0);
        material->diffuse->Bind(0);
    }
    shader->SetVector3("u_Material.specular", material->specular);
    shader->SetFloat("u_Material.shininess", material->shininess);

    vertexArray->Bind();
    vertexArray->Draw();

    vertexArray->Unbind();
    shader->Unbind();
}

void Mesh::Render() const
{
    Render(math::Matrix4::Identity(), RenderContext{});
}

}
}
