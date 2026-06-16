# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific flags
if(MSVC)
    add_compile_options(/W4 /utf-8)

    # Windows GUI application — inherent to SGKit
    add_compile_definitions(
        _WINDOWS
        UNICODE
        _UNICODE
        NOMINMAX
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
    )

    string(APPEND CMAKE_CXX_FLAGS_DEBUG   " /Od /Zi")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /O2 /DNDEBUG")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)

    string(APPEND CMAKE_CXX_FLAGS_DEBUG   " -O0 -g")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O2 -DNDEBUG")
endif()

# Debug-only
add_compile_definitions($<$<CONFIG:Debug>:SGK_DEBUG>)
