#include <sgkit/graphics/Renderer.h>

#include <sgkit/core/DebugOut.h>
#include <glad/glad.h>

static sgkit::graphics::Renderer* g_Renderer = nullptr;

namespace sgkit {
namespace graphics {

void Renderer::Create()
{
    if (g_Renderer) return;
    g_Renderer = new Renderer;
    core::DebugOut("[ SGKit Renderer ]: module created.");
}

void Renderer::Destroy()
{
    if (!g_Renderer) return;
    delete g_Renderer;
    g_Renderer = nullptr;
    core::DebugOut("[ SGKit Renderer ]: module destroyed.");
}

Renderer& Renderer::instance()
{
    return *g_Renderer;
}

void Renderer::SetClearColor(const math::Vector4& color)
{
    m_clearColor = color;
    glClearColor(color.x, color.y, color.z, color.w);
}

void Renderer::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::Draw(const Mesh& mesh)
{
    mesh.Render();
}

void Renderer::Draw(const Mesh& mesh, const math::Matrix4& model, const RenderContext& ctx)
{
    mesh.Render(model, ctx);
}

void Renderer::Draw(const VertexArray& va)
{
    va.Draw();
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void Renderer::SetWireframe(bool enabled)
{
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
}

void Renderer::SetDepthTest(bool enabled)
{
    if (enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void Renderer::SetBlend(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

void Renderer::SetCullFace(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

} // namespace graphics
} // namespace sgkit
