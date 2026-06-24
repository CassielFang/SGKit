#include <sgkit/scene/Scene.h>

#include <glad/glad.h>

#include <cstdio>

namespace sgkit {
namespace scene {

Scene::Scene() {}

Scene::~Scene() {}

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

void Scene::OnRender(graphics::Renderer& renderer, Entity cameraEntity,
                     int viewportWidth, int viewportHeight)
{
    Camera* cam = m_cameras.Get(cameraEntity);
    Transform* camTransform = m_transforms.Get(cameraEntity);
    if (!cam) return;

    float aspect = (viewportHeight > 0) ? static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight) : 1.0f;

    math::Matrix4 viewMatrix = math::Matrix4::Identity();
    if (camTransform)
        viewMatrix = cam->GetViewMatrix(GetWorldMatrix(cameraEntity));
    math::Matrix4 projMatrix = cam->GetProjectionMatrix(aspect);
    math::Matrix4 ViewProjection = projMatrix * viewMatrix;

    glViewport(0, 0, viewportWidth, viewportHeight);
    renderer.Clear();

    Light* light = nullptr;
    Transform* lightTransform = nullptr;

    for (Entity& e : m_aliveEntities)
    {
        light = m_lights.Get(e);
        if (light)
        {
            lightTransform = m_transforms.Get(e);
            break;
        }
    }

    for (Entity& e : m_aliveEntities)
    {
        //MeshRenderer* mr = m_meshRenderers.Get(entity);
        //if (!mr || !mr->enabled || !mr->mesh || !mr->mesh->material || !mr->mesh->material->shader)
        //    continue;
        //Transform* tf = m_transforms.Get(entity);
        //if (!tf) continue;

        //math::Matrix4 mvp = projMatrix * viewMatrix * GetWorldMatrix(entity);

        //auto& shader = mr->mesh->material->shader;
        //shader->Bind();
        //shader->SetMatrix4("u_ModelViewProjection", mvp);

        //mr->mesh->Render();
        MeshRenderer* mr = m_meshRenderers.Get(e);
        if (mr && mr->enabled && mr->mesh && mr->mesh->material && mr->mesh->material->shader) {
            auto& shader = mr->mesh->material->shader;
            shader->Bind();
            shader->SetMatrix4("u_Model", GetWorldMatrix(e));
            shader->SetMatrix4("u_ViewProjection", ViewProjection);
            if (light && lightTransform && camTransform)
            {
                shader->SetVector3("u_cameraPos", camTransform->position);
                shader->SetVector3("u_Light.position", lightTransform->position);
                shader->SetVector3("u_Light.ambient", light->ambient);
                shader->SetVector3("u_Light.diffuse", light->diffuse);
                shader->SetVector3("u_Light.specular", light->specular);
            }

            mr->mesh->Render();
        }
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

} // namespace scene
} // namespace sgkit
