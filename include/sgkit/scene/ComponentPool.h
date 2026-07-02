#pragma once

#include <sgkit/scene/Entity.h>

#include <vector>
#include <algorithm>

namespace sgkit {
namespace scene {

// Sparse set component storage: O(1) add/remove, linear iteration.
template<typename T>
class ComponentPool
{
public:
    T* Add(Entity entity);

    void Remove(Entity entity);

    T* Get(Entity entity);

    const T* Get(Entity entity) const;

    bool Has(Entity entity) const;

    const std::vector<T>& GetComponents() const;
    const std::vector<Entity>& GetEntities() const;

    size_t Size() const;

private:
    std::vector<T>      m_components; // dense component array
    std::vector<size_t> m_sparse;     // sparse: entity -> index
    std::vector<Entity> m_dense;      // dense: index -> entity

    void EnsureSize(size_t size);
};

}
}

// Template implementations
#include <sgkit/scene/ComponentPoolImpl.h>
