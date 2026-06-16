#include <sgkit/framework/Timing.h>

namespace sgkit {
namespace framework {

Clock::Clock()
{
    Restart();
}

void Clock::Restart()
{
    m_start = std::chrono::high_resolution_clock::now();
}

float Clock::GetElapsedSeconds() const
{
    return static_cast<float>(GetElapsedMilliseconds()) / 1000.0f;
}

double Clock::GetElapsedMilliseconds() const
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(now - m_start).count();
}

Clock::TimePoint Clock::Now()
{
    return std::chrono::high_resolution_clock::now();
}

} // namespace framework
} // namespace sgkit
