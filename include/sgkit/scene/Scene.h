#pragma once

#include <sgkit/scene/Entity.h>
#include <sgkit/scene/ComponentPool.h>
#include <sgkit/scene/Components.h>

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

    template<typename T> T*  AddComponent(Entity entity);
    template<typename T> void RemoveComponent(Entity entity);
    template<typename T> T*  GetComponent(Entity entity);
    template<typename T> const T* GetComponent(Entity entity) const;
    template<typename T> bool HasComponent(Entity entity) const;

    void RecomputeWorldTransforms();

    void OnRender(Entity cameraEntity);

private:
    Scene() = default;
    ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(const Scene&&) = delete;
    Scene& operator=(const Scene&&) = delete;

    Entity m_nextEntity = 0;
    std::vector<Entity> m_aliveEntities;

    ComponentPool<Transform>    m_transforms;
    ComponentPool<Camera>       m_cameras;
    ComponentPool<Light>        m_lights;
    ComponentPool<MeshRenderer> m_meshRenderers;

    template<typename T> ComponentPool<T>& GetPool();
    template<typename T> const ComponentPool<T>& GetPool() const;

    int m_width = 0, m_height = 0;
    std::vector<math::Matrix4> m_worldMatrices;
    math::Matrix4 GetWorldMatrix(Entity entity) const;
};

template<typename T> T* Scene::AddComponent(Entity e) { return GetPool<T>().Add(e); }
template<typename T> void Scene::RemoveComponent(Entity e) { GetPool<T>().Remove(e); }
template<typename T> T* Scene::GetComponent(Entity e) { return GetPool<T>().Get(e); }
template<typename T> const T* Scene::GetComponent(Entity e) const { return GetPool<T>().Get(e); }
template<typename T> bool Scene::HasComponent(Entity e) const { return GetPool<T>().Has(e); }

}
}
