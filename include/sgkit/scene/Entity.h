#pragma once

#include <cstdint>

namespace sgkit {
namespace scene {

class Entity {
public:
	Entity();
	Entity(uint32_t id);
	bool operator==(const Entity& other) const;
	uint32_t m_id;
	
	static const Entity Invalid;
};

constexpr uint32_t k_MaxEntities = 10000;

} // namespace scene
} // namespace sgkit
