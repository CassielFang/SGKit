#include <sgkit/scene/Scene.h>

#include <sgkit/core/DebugOut.h>
#include <sgkit/core/Window.h>
#include <sgkit/graphics/Renderer.h>
#include <cstdio>

sgkit::scene::Scene* g_Scene = nullptr;

namespace sgkit {
namespace scene {

void Scene::Create()
{
    if (g_Scene) return;
    g_Scene = new Scene;
    core::DebugOut("[  SGKit Scene   ]: module created.");
}

void Scene::Destroy()
{
    if (!g_Scene) return;
    delete g_Scene;
    g_Scene = nullptr;
    core::DebugOut("[  SGKit Scene   ]: module destroyed.");
}

Scene& Scene::instance()
{
    return *g_Scene;
}

Entity Scene::CreateEntity()
{
    if (m_nextEntity.m_id >= k_MaxEntities)
        return Entity::Invalid;

    Entity entity = m_nextEntity.m_id++;
    m_aliveEntities.push_back(entity);
    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    if (!IsAlive(entity)) return;

    m_transforms.Remove(entity);
    m_cameras.Remove(entity);
    m_lights.Remove(entity);
    m_meshRenderers.Remove(entity);

    auto it = std::find(m_aliveEntities.begin(), m_aliveEntities.end(), entity);
    if (it != m_aliveEntities.end())
        m_aliveEntities.erase(it);
}

bool Scene::IsAlive(Entity entity) const
{
    return std::find(m_aliveEntities.begin(), m_aliveEntities.end(), entity)
           != m_aliveEntities.end();
}

void Scene::RecomputeWorldTransforms()
{
    if (m_worldMatrices.size() <= m_nextEntity.m_id)
        m_worldMatrices.resize(static_cast<size_t>(m_nextEntity.m_id) + 1);

    for (Entity& e : m_aliveEntities)
    {
        Transform* tf = m_transforms.Get(e);
        if (tf)
            m_worldMatrices[e.m_id] = tf->GetLocalMatrix();
        else
            m_worldMatrices[e.m_id] = math::Matrix4::Identity();
    }

    bool changed = true;
    int maxIterations = 100;
    while (changed && maxIterations-- > 0)
    {
        changed = false;
        for (Entity& e : m_aliveEntities)
        {
            Transform* tf = m_transforms.Get(e);
            if (!tf || tf->parent == Entity::Invalid) continue;

            math::Matrix4 parentWorld = m_worldMatrices[tf->parent.m_id];
            math::Matrix4 local = tf->GetLocalMatrix();
            math::Matrix4 world = parentWorld * local;
            if (!(m_worldMatrices[e.m_id] == world))
            {
                m_worldMatrices[e.m_id] = world;
                changed = true;
            }
        }
    }
}

void Scene::OnRender(Entity cameraEntity)
{
    Camera* cam = m_cameras.Get(cameraEntity);
    Transform* camTransform = m_transforms.Get(cameraEntity);
    if (!cam) return;

    core::Window& window = core::Window::instance();
    float aspect = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());

    // Build frame-level render context from camera.
    graphics::RenderContext ctx;
    math::Matrix4 viewMatrix = math::Matrix4::Identity();
    if (camTransform)
    {
        viewMatrix = cam->GetViewMatrix(GetWorldMatrix(cameraEntity));
        ctx.cameraPos = camTransform->position;
    }
    math::Matrix4 projMatrix = cam->GetProjectionMatrix(aspect);
    ctx.viewProjection = projMatrix * viewMatrix;

    // Collect the first light into the context.
    for (Entity& e : m_aliveEntities)
    {
        Light* light = m_lights.Get(e);
        if (light)
        {
            Transform* lt = m_transforms.Get(e);
            ctx.light.position = lt ? lt->position : math::Vector3{};
            ctx.light.ambient  = light->ambient;
            ctx.light.diffuse  = light->diffuse;
            ctx.light.specular = light->specular;
            ctx.hasLight = true;
            break;
        }
    }

    graphics::Renderer::instance().Clear();

    // Draw every entity that has a MeshRenderer.
    // Mesh::Render() handles all uniform setup and the GL draw call.
    for (Entity& e : m_aliveEntities)
    {
        MeshRenderer* mr = m_meshRenderers.Get(e);
        if (mr && mr->enabled && mr->mesh)
            mr->mesh->Render(GetWorldMatrix(e), ctx);
    }
}

template<> ComponentPool<Transform>& Scene::GetPool<Transform>()       { return m_transforms; }
template<> ComponentPool<Camera>& Scene::GetPool<Camera>()             { return m_cameras; }
template<> ComponentPool<Light>& Scene::GetPool<Light>()               { return m_lights; }
template<> ComponentPool<MeshRenderer>& Scene::GetPool<MeshRenderer>() { return m_meshRenderers; }

template<> const ComponentPool<Transform>& Scene::GetPool<Transform>()       const { return m_transforms; }
template<> const ComponentPool<Camera>& Scene::GetPool<Camera>()             const { return m_cameras; }
template<> const ComponentPool<Light>& Scene::GetPool<Light>()               const { return m_lights; }
template<> const ComponentPool<MeshRenderer>& Scene::GetPool<MeshRenderer>() const { return m_meshRenderers; }

math::Matrix4 Scene::GetWorldMatrix(Entity entity) const
{
    if (entity.m_id < m_worldMatrices.size())
        return m_worldMatrices[entity.m_id];
    return math::Matrix4::Identity();
}

}
}
