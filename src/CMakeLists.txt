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
