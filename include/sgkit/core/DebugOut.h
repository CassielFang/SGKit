#pragma once

namespace sgkit {
namespace core {

#ifdef _DEBUG
void DebugOut(const char* str, char end = '\n');
void DebugOut(int code, char end = '\n');
#else
void DebugOut([[maybe_unused]] const char* str, [[maybe_unused]] char end = '\n') {};
void DebugOut([[maybe_unused]] int code, [[maybe_unused]] char end = '\n') {};
#endif

}
}
