#pragma once

#include <sgkit/scene/ComponentPool.h>

namespace sgkit {
namespace scene {

template<typename T>
T* ComponentPool<T>::Add(Entity entity)
{
    size_t idx = entity.m_id;
    EnsureSize(idx + 1);

    if (Has(entity))
        return &m_components[m_sparse[idx]];

    m_sparse[idx] = m_components.size();
    m_dense.push_back(entity);
    m_components.emplace_back();
    return &m_components.back();
}

template<typename T>
void ComponentPool<T>::Remove(Entity entity)
{
    if (!Has(entity.m_id)) return;

    size_t idx = m_sparse[entity.m_id];
    size_t last = m_components.size() - 1;
    Entity lastEntity = m_dense[last];

    // Swap-and-pop
    m_components[idx] = std::move(m_components[last]);
    m_dense[idx] = lastEntity;
    m_sparse[lastEntity.m_id] = idx;

    m_components.pop_back();
    m_dense.pop_back();
    m_sparse[entity.m_id] = 0xFFFFFFFF;
}

template<typename T>
T* ComponentPool<T>::Get(Entity entity)
{
    if (!Has(entity.m_id)) return nullptr;
    return &m_components[m_sparse[entity.m_id]];
}

template<typename T>
const T* ComponentPool<T>::Get(Entity entity) const
{
    if (!Has(entity.m_id)) return nullptr;
    return &m_components[m_sparse[entity.m_id]];
}

template<typename T>
bool ComponentPool<T>::Has(Entity entity) const
{
    if (entity.m_id >= m_sparse.size()) return false;
    size_t idx = m_sparse[entity.m_id];
    return idx != 0xFFFFFFFF && idx < m_dense.size() && m_dense[idx] == entity.m_id;
}

template<typename T>
const std::vector<T>& ComponentPool<T>::GetComponents() const { return m_components; }

template<typename T>
const std::vector<Entity>& ComponentPool<T>::GetEntities() const { return m_dense; }

template<typename T>
size_t ComponentPool<T>::Size() const { return m_components.size(); }

template<typename T>
void ComponentPool<T>::EnsureSize(size_t size)
{
    if (m_sparse.size() < size)
        m_sparse.resize(size, 0xFFFFFFFF);
}

}
}
