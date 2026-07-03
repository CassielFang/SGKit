#include <sgkit/scene/Scene.h>

#include <sgkit/core/DebugOut.h>
#include <sgkit/core/Window.h>

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

math::Matrix4 Scene::GetWorldMatrix(Entity entity) const
{
    if (entity.m_id < m_worldMatrices.size())
        return m_worldMatrices[entity.m_id];
    return math::Matrix4::Identity();
}

// -- Render pipeline

graphics::RenderQueue Scene::BuildRenderQueue()
{
    graphics::RenderQueue queue;

    for (Entity e : m_aliveEntities)
    {
        MeshRenderer* mr = m_meshRenderers.Get(e);
        if (mr && mr->enabled && mr->mesh)
            queue.Submit(mr->mesh, GetWorldMatrix(e));
    }

    return queue;
}

std::vector<graphics::LightData> Scene::CollectLights()
{
    std::vector<graphics::LightData> lights;

    for (Entity e : m_aliveEntities)
    {
        Light* light = m_lights.Get(e);
        if (light)
        {
            Transform* lt = m_transforms.Get(e);

            graphics::LightData data;
            data.position = lt ? lt->position : math::Vector3{};
            data.ambient  = light->ambient;
            data.diffuse  = light->diffuse;
            data.specular = light->specular;
            lights.push_back(data);
        }
    }

    return lights;
}

void Scene::Render(Entity cameraEntity)
{
    Camera*    cam          = m_cameras.Get(cameraEntity);
    Transform* camTransform = m_transforms.Get(cameraEntity);
    if (!cam) return;

    // 1. Build and sort the render queue.
    graphics::RenderQueue queue = BuildRenderQueue();
    math::Vector3 cameraPos = camTransform ? camTransform->position : math::Vector3{0.0f, 0.0f, 0.0f};
    queue.Sort(cameraPos);

    // 2. Camera matrices.
    core::Window& window = core::Window::instance();
    float aspect = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());

    math::Matrix4 viewMatrix = math::Matrix4::Identity();
    if (camTransform)
        viewMatrix = cam->GetViewMatrix(GetWorldMatrix(cameraEntity));
    math::Matrix4 projMatrix = cam->GetProjectionMatrix(aspect);

    // 3. Feed frame data to the renderer.
    graphics::Renderer& renderer = graphics::Renderer::instance();
    renderer.SetViewProjection(projMatrix * viewMatrix);
    renderer.SetCameraPosition(cameraPos);
    renderer.SetAmbientLight({0.1f, 0.1f, 0.15f});
    renderer.SetLights(CollectLights());

    // 4. Draw.
    renderer.Clear();
    renderer.Execute(queue);
}

// -- Template specialisations

template<> ComponentPool<Transform>& Scene::GetPool<Transform>()       { return m_transforms; }
template<> ComponentPool<Camera>& Scene::GetPool<Camera>()             { return m_cameras; }
template<> ComponentPool<Light>& Scene::GetPool<Light>()               { return m_lights; }
template<> ComponentPool<MeshRenderer>& Scene::GetPool<MeshRenderer>() { return m_meshRenderers; }

template<> const ComponentPool<Transform>& Scene::GetPool<Transform>()       const { return m_transforms; }
template<> const ComponentPool<Camera>& Scene::GetPool<Camera>()             const { return m_cameras; }
template<> const ComponentPool<Light>& Scene::GetPool<Light>()               const { return m_lights; }
template<> const ComponentPool<MeshRenderer>& Scene::GetPool<MeshRenderer>() const { return m_meshRenderers; }

}
}
