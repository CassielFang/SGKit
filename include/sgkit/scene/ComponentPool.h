#pragma once

#include <sgkit/scene/Entity.h>

#include <vector>
#include <cstddef>
#include <algorithm>

namespace sgkit {
namespace scene {

// Sparse set component storage: O(1) add/remove, linear iteration.
template<typename T>
class ComponentPool
{
public:
    T& Add(Entity entity)
    {
        size_t idx = entity;
        EnsureSize(idx + 1);

        if (Has(entity))
            return m_components[m_sparse[idx]];

        m_sparse[idx] = m_components.size();
        m_dense.push_back(entity);
        m_components.emplace_back();
        return m_components.back();
    }

    void Remove(Entity entity)
    {
        if (!Has(entity)) return;

        size_t idx  = m_sparse[entity];
        size_t last = m_components.size() - 1;
        Entity lastEntity = m_dense[last];

        // Swap-and-pop
        m_components[idx] = std::move(m_components[last]);
        m_dense[idx]      = lastEntity;
        m_sparse[lastEntity] = idx;

        m_components.pop_back();
        m_dense.pop_back();
        m_sparse[entity] = 0xFFFFFFFF;
    }

    T* Get(Entity entity)
    {
        if (!Has(entity)) return nullptr;
        return &m_components[m_sparse[entity]];
    }

    const T* Get(Entity entity) const
    {
        if (!Has(entity)) return nullptr;
        return &m_components[m_sparse[entity]];
    }

    bool Has(Entity entity) const
    {
        if (entity >= m_sparse.size()) return false;
        size_t idx = m_sparse[entity];
        return idx != 0xFFFFFFFF && idx < m_dense.size() && m_dense[idx] == entity;
    }

    const std::vector<T>&      GetComponents() const { return m_components; }
    const std::vector<Entity>& GetEntities()   const { return m_dense; }

    size_t Size() const { return m_components.size(); }

private:
    std::vector<T>      m_components;      // dense component array
    std::vector<size_t> m_sparse;           // sparse: entity → index
    std::vector<Entity> m_dense;            // dense: index → entity

    void EnsureSize(size_t size)
    {
        if (m_sparse.size() < size)
            m_sparse.resize(size, 0xFFFFFFFF);
    }
};

} // namespace scene
} // namespace sgkit
