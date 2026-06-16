cmake_minimum_required(VERSION 3.20)
project(SGKit VERSION 0.1.0 LANGUAGES CXX C)

include(cmake/Platform.cmake)
include(cmake/CompilerSettings.cmake)

# Third-party
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/glad/src/glad.c")
    message(FATAL_ERROR
        "glad not found at external/glad/src/glad.c. "
        "Generate: python -m glad --api gl=4.6 --profile core --generator c --out-path external/glad/tmp")
endif()
add_subdirectory(external/glad)

# Engine library
add_subdirectory(src)

# Example projects
option(SGK_BUILD_EXAMPLES "Build example projects" ON)
if(SGK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Unit tests
option(SGK_BUILD_TESTS "Build unit tests" ON)
if(SGK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
