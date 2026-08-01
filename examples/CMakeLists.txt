# Example applications — each lives in its own subdirectory

set(ASSIMP_DLL ${CMAKE_SOURCE_DIR}/external/assimp/bin/assimp-vc145-mt.dll)

# Shared helper — every example calls this once
function(sgkit_setup_example NAME)
    target_link_libraries(${NAME} PRIVATE sgkit)

    # Application icon (must be on the exe, not the static lib)
    target_sources(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/icon/app.rc)
    target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/icon)

    # IDE: run from source dir so shader edits don't need a rebuild
    set_target_properties(${NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Copy assimp DLL alongside the executable
    add_custom_command(TARGET ${NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ASSIMP_DLL}
            $<TARGET_FILE_DIR:${NAME}>/
        COMMENT "Copying assimp.dll -> $<TARGET_FILE_DIR:${NAME}>/"
    )

    # Copy assets/ if present
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
        add_custom_command(TARGET ${NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/assets
                $<TARGET_FILE_DIR:${NAME}>/assets
            COMMENT "Copying assets -> $<TARGET_FILE_DIR:${NAME}>/assets"
        )
    endif()
endfunction()

add_subdirectory(Light)
add_subdirectory(AssimpExample)
