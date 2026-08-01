cmake_minimum_required(VERSION 3.20)

# Project
project(SGKit
    VERSION     1.0.0
    DESCRIPTION "Straightforward Graphics Kit — lightweight C++20 3D rendering engine"
    LANGUAGES   C CXX
)

# Global compile settings
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# CRT linkage
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Subdirectories
add_subdirectory(src)
add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)
