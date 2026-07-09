#pragma once

#include <cstdint>

namespace sgkit {
namespace framework {

enum class LogLevel : uint8_t
{
    Info,
    Warn,
    Error,
    Fatal
};

#ifdef _DEBUG
void LogMessage(LogLevel level, const char* category,
                const char* file, int line,
                const char* format, ...);
#else

// Release no-op - must be inline to avoid unresolved external symbol.
inline void LogMessage(LogLevel, const char*, const char*, int, const char*, ...) {}

#endif

}
}

#ifdef _DEBUG

#define SGK_LOG_INFO(cat, fmt, ...)  \
    sgkit::framework::LogMessage(sgkit::framework::LogLevel::Info, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define SGK_LOG_WARN(cat, fmt, ...)  \
    sgkit::framework::LogMessage(sgkit::framework::LogLevel::Warn, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define SGK_LOG_ERROR(cat, fmt, ...) \
    sgkit::framework::LogMessage(sgkit::framework::LogLevel::Error, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define SGK_LOG_FATAL(cat, fmt, ...) \
    sgkit::framework::LogMessage(sgkit::framework::LogLevel::Fatal, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define SGK_ASSERT(cond, msg)                                                          \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            sgkit::framework::LogMessage(sgkit::framework::LogLevel::Fatal, "ASSERT",  \
                                    __FILE__, __LINE__,                                \
                                    "ASSERTION FAILED: " #cond " -- " msg);            \
            __debugbreak();                                                            \
        }                                                                              \
    } while (0)

#else

#define SGK_LOG_INFO(cat, fmt, ...)    ((void)0)
#define SGK_LOG_WARN(cat, fmt, ...)    ((void)0)
#define SGK_LOG_ERROR(cat, fmt, ...)   ((void)0)
#define SGK_LOG_FATAL(cat, fmt, ...)   ((void)0)

#define SGK_ASSERT(cond, msg)          ((void)0)

#endif
