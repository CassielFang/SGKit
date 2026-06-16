#pragma once

#include <cstdint>

namespace sgkit {
namespace scene {

using Entity = uint32_t;

constexpr Entity k_InvalidEntity = 0xFFFFFFFF;
constexpr Entity k_MaxEntities   = 10000;

} // namespace scene
} // namespace sgkit
