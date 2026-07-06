#include <sgkit/core/DebugOut.h>

#include <cstdio>
#include <Windows.h>

namespace sgkit {
namespace core {
    
#ifdef _DEBUG
void DebugOut(const char* str, char end)
{
    char buff[1024]{};
    sprintf_s(buff, 1024, "%s%c", str, end);
    std::fprintf(stderr, buff);
#ifdef _WINDOWS
    OutputDebugStringA(buff);
#endif
}
void DebugOut(int code, char end)
{
    char buff[1024]{};
    sprintf_s(buff, 1024, "%d%c", code, end);
    std::fprintf(stderr, buff);
#ifdef _WINDOWS
    OutputDebugStringA(buff);
#endif
}
#else
void DebugOut([[maybe_unused]] const char* str, [[maybe_unused]] char end) {}
void DebugOut([[maybe_unused]] int code, [[maybe_unused]] char end) {}
#endif

}
}
