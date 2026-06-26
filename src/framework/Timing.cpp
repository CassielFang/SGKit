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
    return static_cast<float>(GetElapsedMilliseconds() / 1000.0);
}

double Clock::GetElapsedMilliseconds() const
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(now - m_start).count();
}

Clock::TimePoint Clock::g_startTime = std::chrono::high_resolution_clock::now();
Clock::TimePoint Clock::g_nowTime;
Clock::TimePoint Clock::g_lastFrameTime;
float Clock::g_deltaTime = 0.0f;
float Clock::g_fpsTimer = 0.0f;
int   Clock::g_frameCount = 0;
float Clock::g_fps = 0.0f;

float Clock::NowElapsedSeconds()
{
    return static_cast<float>(NowElapsedMilliseconds() / 1000.0);
}

double Clock::NowElapsedMilliseconds()
{
    return std::chrono::duration<double, std::milli>(g_nowTime - g_startTime).count();
}

float Clock::GetFrameDeltaSeconds()
{
    return g_deltaTime;
}

float Clock::GetFPS()
{
    return g_fps;
}

void Clock::Update()
{
    g_nowTime = std::chrono::high_resolution_clock::now();
    g_deltaTime = std::chrono::duration<float>(g_nowTime - g_lastFrameTime).count();
    g_lastFrameTime = g_nowTime;

    g_fpsTimer += g_deltaTime;
    ++g_frameCount;
    if (g_fpsTimer > 0.5f)
    {
        g_fps = g_frameCount / g_fpsTimer;
        g_fpsTimer = 0.0f;
        g_frameCount = 0;
    }
}

} // namespace framework
} // namespace sgkit
