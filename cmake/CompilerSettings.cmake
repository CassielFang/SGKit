# -- C++ standard ---------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -- Global definitions ----------------------------------------------
add_compile_definitions(
    UNICODE              # Windows API wide-character
    _UNICODE             # CRT wide-character
    NOMINMAX             # prevent windows.h min/max macros
    WINVER=0x0A00        # target Windows 10+
    _WIN32_WINNT=0x0A00
)

# -- Compiler-specific options ---------------------------------------
if(MSVC)
    add_compile_options(/utf-8 /W4)

    add_compile_options(
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Release>:/O2>
    )
else()
    add_compile_options(-Wall -Wextra -Wpedantic)

    add_compile_options(
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Release>:-DNDEBUG>
    )
endif()
