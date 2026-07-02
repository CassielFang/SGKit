#pragma once

#include <cmath>

namespace sgkit {
namespace math {

// -- Scalar constants
constexpr float k_Pi        = 3.1415927f;
constexpr float k_TwoPi     = 6.2831855f;
constexpr float k_HalfPi    = 1.5707963f;
constexpr float k_Epsilon   = 1e-5f;
constexpr float k_Deg2Rad   = k_Pi / 180.0f;
constexpr float k_Rad2Deg   = 180.0f / k_Pi;

inline float ToRadians(float degrees) { return degrees * k_Deg2Rad; }
inline float ToDegrees(float radians) { return radians * k_Rad2Deg; }

inline bool Approximately(float a, float b, float epsilon = k_Epsilon)
{
    return std::fabs(a - b) < epsilon;
}

template<typename T>
inline T Clamp(T value, T minVal, T maxVal)
{
    return (value < minVal) ? minVal : ((value > maxVal) ? maxVal : value);
}

template<typename T>
inline T Lerp(T a, T b, float t)
{
    return a + (b - a) * t;
}

}
}
