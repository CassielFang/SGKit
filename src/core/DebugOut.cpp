#include <sgkit/core/DebugOut.h>

#include <cstdio>
#include <Windows.h>

namespace sgkit {
namespace core {
    
#ifdef _DEBUG
void DebugOut(const char* str, char end)
{
    char buff[512]{};
    sprintf_s(buff, 512, "%s%c", str, end);
    std::fprintf(stderr, buff);
#ifdef _WINDOWS
    OutputDebugStringA(buff);
#endif
}
void DebugOut(int code, char end)
{
    char buff[512]{};
    sprintf_s(buff, 512, "%d%c", code, end);
    std::fprintf(stderr, buff);
#ifdef _WINDOWS
    OutputDebugStringA(buff);
#endif
}
#endif

}
}
