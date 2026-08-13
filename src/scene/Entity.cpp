#include <sgkit/scene/Entity.h>

namespace sgkit {
namespace scene {

const Entity Entity::Invalid;

Entity::Entity() : m_id(0xFFFFFFFF) {}

Entity::Entity(uint32_t id) : m_id(id) {}

Entity::Entity(const Entity& other) : m_id(other.m_id) {}

Entity& Entity::operator=(const Entity& other)
{
    m_id = other.m_id;
    return *this;
}

bool Entity::operator==(const Entity& other) const
{
    return m_id == other.m_id;
}

bool Entity::operator!=(const Entity& other) const
{
    return m_id != other.m_id;
}

}
}
