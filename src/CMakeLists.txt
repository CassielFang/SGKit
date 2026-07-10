# -- Collect sources by module
file(GLOB_RECURSE SGK_SRC
    "core/*.cpp"
    "math/*.cpp"
    "graphics/*.cpp"
    "scene/*.cpp"
    "framework/*.cpp"
)

# Scripting sources compile into the engine only when scripting is enabled.
if(SGK_ENABLE_SCRIPTING)
    file(GLOB_RECURSE SGK_SCRIPTING_SRC "scripting/*.cpp")
    list(APPEND SGK_SRC ${SGK_SCRIPTING_SRC})
endif()

# -- Engine static library
add_library(sgkit STATIC ${SGK_SRC})

target_include_directories(sgkit PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(sgkit PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/assimp/include>
)

# glad header and resource visible only inside sgkit, not to consumers
target_include_directories(sgkit PRIVATE ${CMAKE_SOURCE_DIR}/icon)

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

# -- C# scripting: vendored .NET host (nethost/hostfxr) - PRIVATE so the
#    vendored headers never leak onto a consumer's include path.
if(SGK_ENABLE_SCRIPTING)
    target_include_directories(sgkit PRIVATE ${CMAKE_SOURCE_DIR}/external/dotnet/include)
    target_link_libraries(sgkit PRIVATE ${CMAKE_SOURCE_DIR}/external/dotnet/lib/nethost.lib)
    target_compile_definitions(sgkit PRIVATE SGK_HAS_SCRIPTING)
endif()

# -- Link dependencies
target_link_libraries(sgkit PRIVATE glad stb ${SGK_PLATFORM_LIBS})
target_link_libraries(sgkit PRIVATE ${CMAKE_SOURCE_DIR}/external/assimp/lib/assimp-vc145-mt.lib)

# -- Output filename: Debug -> sgkit_d.lib, Release -> sgkit.lib ----------
set_target_properties(sgkit PROPERTIES
    DEBUG_OUTPUT_NAME   sgkit_d
    RELEASE_OUTPUT_NAME sgkit
)

# -- Copy .lib (and .pdb) to lib/ for standalone distribution ------------------------
add_custom_command(TARGET sgkit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sgkit>
        ${CMAKE_SOURCE_DIR}/lib/
    COMMAND ${CMAKE_COMMAND} -E
        $<IF:$<CONFIG:Debug>,copy_if_different,true>
        $<IF:$<CONFIG:Debug>,$<TARGET_FILE_DIR:sgkit>/$<TARGET_FILE_BASE_NAME:sgkit>.pdb,true>
        $<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/lib/,true>
)

