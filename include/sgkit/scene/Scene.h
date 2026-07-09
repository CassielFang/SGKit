#pragma once

#include <sgkit/scene/Entity.h>
#include <sgkit/scene/ComponentPool.h>
#include <sgkit/scene/Components.h>
#include <sgkit/scene/RenderQueue.h>
#include <sgkit/scene/Renderer.h>

namespace sgkit {
namespace scene {

class Scene
{
public:
    static void Create();
    static void Destroy();
    static Scene& instance();

    Entity CreateEntity();
    void   DestroyEntity(Entity entity);
    bool   IsAlive(Entity entity) const;

    void   SetEnabled(Entity entity, bool enabled);

    template<typename T> T*       AddComponent(Entity entity);
    template<typename T> void     RemoveComponent(Entity entity);
    template<typename T> T*       GetComponent(Entity entity);
    template<typename T> const T* GetComponent(Entity entity) const;
    template<typename T> bool     HasComponent(Entity entity) const;

    void RecomputeWorldTransforms();
    math::Matrix4 GetWorldMatrix(Entity entity) const;

    RenderQueue BuildRenderQueue();
    std::vector<LightInstance> CollectLights();
    void Render(Entity cameraEntity);

private:
    Scene() = default;
    ~Scene() = default;
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    Entity m_nextEntity = 0;
    std::vector<Entity> m_aliveEntities;

    ComponentPool<component::Transform>    m_transforms;
    ComponentPool<component::Camera>       m_cameras;
    ComponentPool<component::Light>        m_lights;
    ComponentPool<component::MeshRenderer> m_meshRenderers;

    template<typename T> ComponentPool<T>& GetPool();
    template<typename T> const ComponentPool<T>& GetPool() const;

    std::vector<math::Matrix4> m_worldMatrices;
};

template<typename T> T*       Scene::AddComponent(Entity e)       { return GetPool<T>().Add(e); }
template<typename T> void     Scene::RemoveComponent(Entity e)    { GetPool<T>().Remove(e); }
template<typename T> T*       Scene::GetComponent(Entity e)       { return GetPool<T>().Get(e); }
template<typename T> const T* Scene::GetComponent(Entity e) const { return GetPool<T>().Get(e); }
template<typename T> bool     Scene::HasComponent(Entity e) const { return GetPool<T>().Has(e); }

}
}
