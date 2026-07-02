#include <sgkit/scene/Entity.h>

namespace sgkit {
namespace scene {

const Entity Entity::Invalid;

Entity::Entity() : m_id(0xFFFFFFFF) {}
Entity::Entity(uint32_t id) : m_id(id) {}

bool Entity::operator==(const Entity& other) const { return m_id == other.m_id; }

}
}
