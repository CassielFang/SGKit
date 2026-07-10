# Builds the managed (C#) side via `dotnet build` and exposes its output
# directory so example targets can copy the assemblies next to their exe.
#
# SGKit is a static lib linked into each example exe, so the engine singletons
# (Scene, Input, ...) live in the exe. The C# host therefore also runs in the
# exe process and reaches native code through a function-pointer table rather
# than a separate interop DLL (which would duplicate those singletons).

find_program(DOTNET_EXE NAMES dotnet REQUIRED)

set(SGK_MANAGED_PROJ ${CMAKE_CURRENT_SOURCE_DIR}/SGKit.Managed.csproj)
set(SGK_MANAGED_OUT  ${CMAKE_CURRENT_BINARY_DIR}/out/$<CONFIG>)

# CMake Debug/Release -> dotnet Debug/Release
set(_SGK_DN_CFG $<IF:$<CONFIG:Debug>,Debug,Release>)

# `-E env DOTNET_CLI_UI_LANGUAGE=en` keeps dotnet's output ASCII so the VS
# output pane (which decodes it with the system code page) shows it cleanly.
# This builds only the engine's managed API. Per-example game-script assemblies
# (that reference SGKit.Managed) are built by each example's own CMakeLists.
add_custom_target(SGKitManaged ALL
    COMMAND ${CMAKE_COMMAND} -E env DOTNET_CLI_UI_LANGUAGE=en
            ${DOTNET_EXE} build "${SGK_MANAGED_PROJ}"
            -c ${_SGK_DN_CFG} -o "${SGK_MANAGED_OUT}" --nologo -v quiet
    BYPRODUCTS
        ${SGK_MANAGED_OUT}/SGKit.Managed.dll
        ${SGK_MANAGED_OUT}/SGKit.Managed.runtimeconfig.json
        ${SGK_MANAGED_OUT}/SGKit.Managed.deps.json
    COMMENT "Building SGKit.Managed (C#) [${_SGK_DN_CFG}]"
    VERBATIM
)

# Global so any example subdirectory can copy from it regardless of add order.
set(SGK_MANAGED_OUTPUT_DIR "${SGK_MANAGED_OUT}" CACHE INTERNAL "SGKit managed build output dir")

# Optional: regenerate the interop bindings (NativeApi.gen.h + Bindings.gen.cs)
# from Interop.h. NOT part of ALL - the generated files are committed to the
# repo, so normal builds never need Python. Build the "SGKitBindings" target in
# VS (or run the script directly) only after editing the interop surface.
find_program(PYTHON_EXE NAMES python python3)
if(PYTHON_EXE)
    add_custom_target(SGKitBindings
        COMMAND ${PYTHON_EXE} ${CMAKE_SOURCE_DIR}/tools/generate_bindings.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Regenerating C#/C++ interop bindings from Interop.h"
        VERBATIM
    )
endif()
