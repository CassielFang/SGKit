#include <sgkit/math/Vector2.h>
#include <sgkit/math/Vector3.h>
#include <sgkit/math/Vector4.h>

namespace sgkit {
namespace math {

const Vector2 Vector2::k_Zero  = { 0.0f, 0.0f };
const Vector2 Vector2::k_One   = { 1.0f, 1.0f };
const Vector2 Vector2::k_Up    = { 0.0f, 1.0f };
const Vector2 Vector2::k_Right = { 1.0f, 0.0f };

const Vector3 Vector3::k_Zero    = {  0.0f,  0.0f,  0.0f  };
const Vector3 Vector3::k_One     = {  1.0f,  1.0f,  1.0f  };
const Vector3 Vector3::k_Up      = {  0.0f,  1.0f,  0.0f  };
const Vector3 Vector3::k_Down    = {  0.0f, -1.0f,  0.0f  };
const Vector3 Vector3::k_Right   = {  1.0f,  0.0f,  0.0f  };
const Vector3 Vector3::k_Left    = { -1.0f,  0.0f,  0.0f  };
const Vector3 Vector3::k_Forward = {  0.0f,  0.0f, -1.0f  };
const Vector3 Vector3::k_Back    = {  0.0f,  0.0f,  1.0f  };

const Vector4 Vector4::k_Zero = { 0.0f, 0.0f, 0.0f, 0.0f };
const Vector4 Vector4::k_One  = { 1.0f, 1.0f, 1.0f, 1.0f };

}
}
