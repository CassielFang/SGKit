#pragma once

#include <sgkit/math/Vector3.h>
#include <sgkit/math/Quaternion.h>
#include <sgkit/math/Matrix4.h>

namespace sgkit {
namespace math {

struct Transform
{
    Vector3    position{0.0f, 0.0f, 0.0f};
    Quaternion rotation{};
    Vector3    scale{1.0f, 1.0f, 1.0f};

    Transform() = default;

    Matrix4 GetLocalMatrix() const;
};

} // namespace math
} // namespace sgkit
