#pragma once

#include <chrono>

namespace sgkit {
namespace framework {

class Clock
{
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;

    Clock();

    void   Restart();
    float  GetElapsedSeconds() const;
    double GetElapsedMilliseconds() const;

    static float NowElapsedSeconds();
    static double NowElapsedMilliseconds();
    static float GetFrameDeltaSeconds();
    static float GetFPS();
    static void Update();

private:
    Clock(const Clock&) = delete;
    Clock& operator=(const Clock&) = delete;
    Clock(const Clock&&) = delete;
    Clock& operator=(const Clock&&) = delete;

    TimePoint m_start;

    static TimePoint g_startTime;
    static TimePoint g_nowTime;
    static TimePoint g_lastFrameTime;
    static float     g_deltaTime;
    static float     g_fpsTimer;
    static int       g_frameCount;
    static float     g_fps;
};

}
}
