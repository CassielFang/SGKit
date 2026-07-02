# -- Collect sources by module
file(GLOB_RECURSE SGK_SRC 
    "math/*.cpp"
    "core/*.cpp"
    "graphics/*.cpp"
    "scene/*.cpp"
    "framework/*.cpp"
)

# -- Engine static library
add_library(sgkit STATIC ${SGK_SRC})

target_include_directories(sgkit PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
# glad header and resource visible only inside sgkit, not to consumers
target_include_directories(sgkit PRIVATE ${PROJECT_SOURCE_DIR}/icon)

# -- Compiler-specific options
if(MSVC)
    target_compile_options(sgkit PRIVATE /utf-8 /W4)

    target_compile_options(sgkit PRIVATE
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Release>:/O2>
    )
endif()

# -- Internal platform macro (sgkit PRIVATE only)
target_compile_definitions(sgkit PRIVATE _WINDOWS)

# -- Link dependencies
target_link_libraries(sgkit PUBLIC glad ${SGK_PLATFORM_LIBS})

# -- Output filename: Debug -> sgkit_d.lib, Release -> sgkit.lib ----------
set_target_properties(sgkit PROPERTIES
    DEBUG_OUTPUT_NAME   sgkit_d
    RELEASE_OUTPUT_NAME sgkit
)

# -- Copy .lib (and .pdb) to lib/ for standalone distribution ------------------------
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
else()
add_custom_command(TARGET sgkit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sgkit>
        ${CMAKE_SOURCE_DIR}/lib/
)
endif()
