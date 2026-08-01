# SGKit static library
add_library(sgkit STATIC)

# Public include path (consumers only see SGKit's own headers)
target_include_directories(sgkit PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Private include paths (third-party, internal to the library)
target_include_directories(sgkit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/glad/include
    ${CMAKE_SOURCE_DIR}/external/stb/include
    ${CMAKE_SOURCE_DIR}/external/assimp/include
    ${CMAKE_SOURCE_DIR}/icon   # for resource.h (IDI_MAIN_ICON used by Window.cpp)
)

# Auto-discover library sources
file(GLOB_RECURSE SGKIT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
file(GLOB_RECURSE SGKIT_THIRD_PARTY
    ${CMAKE_SOURCE_DIR}/external/glad/src/*.c
    ${CMAKE_SOURCE_DIR}/external/stb/src/*.c
)
target_sources(sgkit PRIVATE
    ${SGKIT_SOURCES}
    ${SGKIT_THIRD_PARTY}
)

# Platform definitions & system libraries
target_compile_definitions(sgkit PRIVATE _WINDOWS UNICODE _UNICODE)

target_link_libraries(sgkit
    PUBLIC
        gdi32 user32 opengl32 imm32
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external/assimp/lib/assimp-vc145-mt.lib
)

# Output: lib/Debug/sgkit.lib  /  lib/Release/sgkit.lib
set_target_properties(sgkit PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/$<CONFIG>
)
