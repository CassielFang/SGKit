#include <sgkit/framework/DebugOut.h>

#include <sgkit/framework/Timing.h>
#include <cstdio>
#include <cstdarg>

#ifdef _WINDOWS
#include <windows.h>
#endif

namespace sgkit {
namespace framework {

#ifdef _DEBUG

static const char* LevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info:  return "INFO ";
    case LogLevel::Warn:  return "WARN ";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    }
    return "?????";
}

void LogMessage(LogLevel level, const char* category,
                const char* file, int line,
                const char* format, ...)
{
    // 1. Format the user message
    char msgBuf[1024]{};
    {
        va_list args;
        va_start(args, format);
        vsprintf_s(msgBuf, 1024, format, args);
        va_end(args);
    }

    // 2. Extract filename from path
    const char* filename = file;
    for (const char* p = file; *p; ++p)
    {
        if (*p == '/' || *p == '\\')
            filename = p + 1;
    }

    // 3. Elapsed time since program start
    double elapsedMs = Clock::NowElapsedMilliseconds();

    // 4. Compose final line
    char output[2048]{};
    sprintf_s(output, 2048, "[%7.2lf][%s][%-10s] %s  (%s:%d)\n",
        elapsedMs / 1000.0, LevelToString(level), category, msgBuf, filename, line);

    // 5. Emit
    std::fprintf(stderr, "%s", output);
#ifdef _WINDOWS
    OutputDebugStringA(output);
#endif
}

#endif

} // namespace framework
} // namespace sgkit
