#include <sgkit/scene/Scene.h>

#include <sgkit/framework/DebugOut.h>
#include <sgkit/core/Window.h>

namespace sgkit {
namespace scene {

using namespace scene::component;

sgkit::scene::Scene* g_Scene = nullptr;

void Scene::Create()
{
    if (g_Scene) return;
    g_Scene = new Scene;
    SGK_LOG_INFO("Scene", "Module created");
}

void Scene::Destroy()
{
    if (!g_Scene) return;
    delete g_Scene;
    g_Scene = nullptr;
    SGK_LOG_INFO("Scene", "Module destroyed");
}

Scene& Scene::instance()
{
    return *g_Scene;
}

Entity Scene::CreateEntity()
{
    if (m_nextEntity.m_id >= k_MaxEntities)
        return Entity::Invalid;

    Entity entity(m_nextEntity.m_id++);
    m_aliveEntities.push_back(entity);
    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    if (!IsAlive(entity)) return;

    // Snapshot children + parent before any modification
    // (cascade removal can swap-and-pop the pool, invalidating raw pointers)
    component::Transform* tf    = m_transforms.Get(entity);
    std::vector<Entity>  kids   = tf ? tf->children : std::vector<Entity>{};
    Entity               parent = tf ? tf->parent : Entity::Invalid;

    // 1. Cascade: destroy children first
    for (Entity child : kids)
        DestroyEntity(child);

    // 2. Unlink from parent (before removing our own Transform)
    if (parent != Entity::Invalid)
    {
        component::Transform* parentTf = m_transforms.Get(parent);
        if (parentTf)
        {
            auto it = std::find(parentTf->children.begin(), parentTf->children.end(), entity);
            if (it != parentTf->children.end())
                parentTf->children.erase(it);
        }
    }

    // 3. Remove components from pools
    m_transforms.Remove(entity);
    m_cameras.Remove(entity);
    m_lights.Remove(entity);
    m_meshRenderers.Remove(entity);

    // 4. Remove from alive list
    auto it = std::find(m_aliveEntities.begin(), m_aliveEntities.end(), entity);
    if (it != m_aliveEntities.end())
        m_aliveEntities.erase(it);
}

void Scene::SetVisible(Entity entity, bool enabled)
{
    component::MeshRenderer* mr = m_meshRenderers.Get(entity);
    if (mr) mr->enabled = enabled;

    component::Transform* tf = m_transforms.Get(entity);
    if (tf)
    {
        for (Entity child : tf->children)
            SetVisible(child, enabled);
    }
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
        component::Transform* tf = m_transforms.Get(e);
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
            component::Transform* tf = m_transforms.Get(e);
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

RenderQueue Scene::BuildRenderQueue()
{
    RenderQueue queue;

    for (Entity e : m_aliveEntities)
    {
        component::MeshRenderer* mr = m_meshRenderers.Get(e);
        if (mr && mr->enabled && mr->mesh)
            queue.Submit(mr->mesh, GetWorldMatrix(e));
    }

    return queue;
}

std::vector<LightInstance> Scene::CollectLights()
{
    std::vector<LightInstance> instances;

    for (Entity e : m_aliveEntities)
    {
        component::Light* lc = m_lights.Get(e);
        if (lc)
        {
            component::Transform* tf = m_transforms.Get(e);

            LightInstance inst;
            inst.worldPosition = tf ? tf->position : math::Vector3{};
            inst.attribute = lc;

            instances.push_back(inst);
        }
    }

    return instances;
}

void Scene::Render(Entity cameraEntity)
{
    component::Camera*    camComp      = m_cameras.Get(cameraEntity);
    component::Transform* camTransform = m_transforms.Get(cameraEntity);
    if (!camComp) return;

    RenderQueue queue = BuildRenderQueue();
    math::Vector3 cameraPos =
        camTransform ? camTransform->position : math::Vector3{0.0f, 0.0f, 0.0f};
    queue.Sort(cameraPos);

    Renderer& renderer = Renderer::instance();

    core::Window& window = core::Window::instance();
    float aspect = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());

    // -- Shadow pass
    {
        // Find first directional light
        component::Light* dirLight = nullptr;
        for (Entity e : m_aliveEntities)
        {
            auto* l = m_lights.Get(e);
            if (l && l->type == component::Light::Type::Directional)
            {
                dirLight = l;
                break;
            }
        }

        if (dirLight)
        {
            math::Matrix4 viewMat = math::Matrix4::Identity();
            if (camTransform)
                viewMat = camComp->GetViewMatrix(GetWorldMatrix(cameraEntity));
            renderer.RenderCSMShadowPass(queue, dirLight->direction,
                                          viewMat,
                                          camComp->GetProjectionMatrix(aspect),
                                          cameraPos,
                                          camComp->nearPlane, camComp->farPlane,
                                          aspect);
        }
    }

    // -- Main render ---------------------------------------------------------
    math::Matrix4 viewMatrix = math::Matrix4::Identity();
    if (camTransform)
        viewMatrix = camComp->GetViewMatrix(GetWorldMatrix(cameraEntity));

    // Extract camera forward from view matrix (column 2 = -forward)
    math::Vector3 camForward{0.0f, 0.0f, -1.0f};
    if (camTransform)
    {
        camForward = math::Vector3{
            -viewMatrix.m[2][0], -viewMatrix.m[2][1], -viewMatrix.m[2][2]
        }.Normalized();
    }

    renderer.SetViewProjection(camComp->GetProjectionMatrix(aspect) * viewMatrix);
    renderer.SetCameraPosition(cameraPos);
    renderer.SetCameraForward(camForward);
    renderer.SetLights(CollectLights());
    renderer.CommitFrameData();   // upload UBO once

    renderer.Clear();
    renderer.Execute(queue);
}

// -- Template specialisations

template<> ComponentPool<component::Transform>&
    Scene::GetPool<component::Transform>()        { return m_transforms; }
template<> ComponentPool<component::Camera>&
    Scene::GetPool<component::Camera>()           { return m_cameras; }
template<> ComponentPool<component::Light>&
    Scene::GetPool<component::Light>()            { return m_lights; }
template<> ComponentPool<component::MeshRenderer>&
    Scene::GetPool<component::MeshRenderer>()     { return m_meshRenderers; }

template<> const ComponentPool<component::Transform>&
    Scene::GetPool<component::Transform>() const    { return m_transforms; }
template<> const ComponentPool<component::Camera>&
    Scene::GetPool<component::Camera>() const       { return m_cameras; }
template<> const ComponentPool<component::Light>&
    Scene::GetPool<component::Light>() const        { return m_lights; }
template<> const ComponentPool<component::MeshRenderer>&
    Scene::GetPool<component::MeshRenderer>() const { return m_meshRenderers; }

}
}
