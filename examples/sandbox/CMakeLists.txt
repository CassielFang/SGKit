add_executable(sandbox WIN32 main.cpp ${CMAKE_SOURCE_DIR}/icon/app.rc)
target_link_libraries(sandbox PRIVATE sgkit)

# Copy assets to build output directory
add_custom_command(TARGET sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:sandbox>/assets
)

# Also copy to project root so they are found from VS "Open Folder" default working directory
add_custom_command(TARGET sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_SOURCE_DIR}/assets
)

set_target_properties(sandbox PROPERTIES
    LINK_FLAGS "/ENTRY:wWinMainCRTStartup"
)
