cmake_minimum_required(VERSION 3.20)
project(SGKit VERSION 0.1.0 LANGUAGES CXX C)

# -- C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -- Global definitions
add_compile_definitions(
    UNICODE              # Windows API wide-character
    _UNICODE             # CRT wide-character
    NOMINMAX             # prevent windows.h min/max macros
    WINVER=0x0A00        # target Windows 10+
    _WIN32_WINNT=0x0A00
)

# -- Windows system libraries
set(SGK_PLATFORM_LIBS  kernel32 user32 gdi32 opengl32 imm32)

# -- externals
add_subdirectory(external)

# -- Engine core
add_subdirectory(src)

# -- Example projects
option(SGK_BUILD_EXAMPLES "Build example projects" ON)
if(SGK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# -- Unit tests
option(SGK_BUILD_TESTS "Build unit tests" ON)
if(SGK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
