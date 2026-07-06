#pragma once

#include <cstdint>

namespace sgkit {
namespace scene {

class Entity
{
public:
	Entity();
	Entity(uint32_t id);
    Entity(const Entity& other);
    Entity& operator=(const Entity& other);

	bool operator==(const Entity& other) const;

	uint32_t m_id;
	
	static const Entity Invalid;
};

constexpr uint32_t k_MaxEntities = 10000;

}
}
