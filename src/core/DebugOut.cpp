#include <sgkit/core/DebugOut.h>

#include <cstdio>

namespace sgkit {
namespace core {
	
#ifdef _DEBUG
void DebugOut(const char* str, char end)
{
	std::fprintf(stderr, "%s%c", str, end);
}
void DebugOut(int code, char end)
{
	std::fprintf(stderr, "%d%c", code, end);
}
#endif

}
}
