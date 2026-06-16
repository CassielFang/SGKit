file(GLOB_RECURSE SGK_MATH_SRC   math/*.cpp)
file(GLOB_RECURSE SGK_CORE_SRC   core/*.cpp)
file(GLOB_RECURSE SGK_GFX_SRC    graphics/*.cpp)
file(GLOB_RECURSE SGK_SCENE_SRC  scene/*.cpp)
file(GLOB_RECURSE SGK_FW_SRC     framework/*.cpp)

add_library(sgkit STATIC
    ${SGK_MATH_SRC}
    ${SGK_CORE_SRC}
    ${SGK_GFX_SRC}
    ${SGK_SCENE_SRC}
    ${SGK_FW_SRC}
)

target_include_directories(sgkit PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_compile_definitions(sgkit PRIVATE SGK_PLATFORM_WINDOWS)
elseif(APPLE)
    target_compile_definitions(sgkit PRIVATE SGK_PLATFORM_MACOS)
else()
    target_compile_definitions(sgkit PRIVATE SGK_PLATFORM_LINUX)
endif()

target_link_libraries(sgkit PUBLIC glad ${SGK_PLATFORM_LIBS})

# Debug → sgkit_d.lib / Release → sgkit.lib
set_target_properties(sgkit PROPERTIES
    DEBUG_OUTPUT_NAME   "sgkit_d"
    RELEASE_OUTPUT_NAME "sgkit"
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Copy .lib + .pdb to project root lib/ for standalone distribution
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET sgkit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sgkit>
            ${CMAKE_SOURCE_DIR}/lib/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:sgkit>/$<TARGET_FILE_BASE_NAME:sgkit>.pdb
            ${CMAKE_SOURCE_DIR}/lib/
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET sgkit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sgkit>
            ${CMAKE_SOURCE_DIR}/lib/
    )
endif()
