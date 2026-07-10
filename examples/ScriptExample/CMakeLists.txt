add_executable(ScriptExample WIN32 main.cpp ${CMAKE_SOURCE_DIR}/icon/app.rc)
target_include_directories(ScriptExample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ScriptExample PRIVATE sgkit)

# -- This example's own game scripts (GameScripts.dll) ------------------------
# Lives next to the example (scripts/) and references the engine's SGKit.Managed.
# DOTNET_EXE is found (and cached) by managed/CMakeLists.txt.
set(SCRIPTS_PROJ ${CMAKE_CURRENT_SOURCE_DIR}/scripts/GameScripts.csproj)
set(SCRIPTS_OUT  ${CMAKE_CURRENT_BINARY_DIR}/scripts_out/$<CONFIG>)
set(_DN_CFG      $<IF:$<CONFIG:Debug>,Debug,Release>)

add_custom_target(ScriptExampleScripts ALL
    COMMAND ${CMAKE_COMMAND} -E env DOTNET_CLI_UI_LANGUAGE=en
            ${DOTNET_EXE} build "${SCRIPTS_PROJ}"
            -c ${_DN_CFG} -o "${SCRIPTS_OUT}" --nologo -v quiet
    BYPRODUCTS ${SCRIPTS_OUT}/GameScripts.dll
    COMMENT "Building GameScripts (C#) for ScriptExample [${_DN_CFG}]"
    VERBATIM
)
# Serialize after SGKitManaged so both don't build SGKit.Managed concurrently.
add_dependencies(ScriptExampleScripts SGKitManaged)
add_dependencies(ScriptExample SGKitManaged ScriptExampleScripts)

# Copy everything the host needs next to the exe: the engine managed API + its
# runtime config (from managed/), this example's scripts, and nethost.dll.
add_custom_command(TARGET ScriptExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SGK_MANAGED_OUTPUT_DIR}/SGKit.Managed.dll
        $<TARGET_FILE_DIR:ScriptExample>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SGK_MANAGED_OUTPUT_DIR}/SGKit.Managed.runtimeconfig.json
        $<TARGET_FILE_DIR:ScriptExample>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SGK_MANAGED_OUTPUT_DIR}/SGKit.Managed.deps.json
        $<TARGET_FILE_DIR:ScriptExample>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SCRIPTS_OUT}/GameScripts.dll
        $<TARGET_FILE_DIR:ScriptExample>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/external/dotnet/bin/nethost.dll
        $<TARGET_FILE_DIR:ScriptExample>
    VERBATIM
)
