# -- Sandbox: basic scene (cubes + floor + camera controls) ----------
add_executable(sandbox WIN32 main.cpp ${CMAKE_SOURCE_DIR}/icon/app.rc)
target_link_libraries(sandbox PRIVATE sgkit)

# Copy assets to exe dir and project root (VS "Open Folder" workdir)
add_custom_command(TARGET sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:sandbox>/assets
)
