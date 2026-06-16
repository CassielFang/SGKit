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

    static TimePoint Now();

private:
    TimePoint m_start;
};

} // namespace framework
} // namespace sgkit
