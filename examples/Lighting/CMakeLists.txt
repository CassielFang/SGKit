add_executable(Lighting WIN32 main.cpp ${CMAKE_SOURCE_DIR}/icon/app.rc)
target_link_libraries(Lighting PRIVATE sgkit)

# Copy assets to build output directory
add_custom_command(TARGET Lighting POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:Lighting>/assets
)

# Also copy to project root so they are found from VS "Open Folder" default working directory
add_custom_command(TARGET Lighting POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_SOURCE_DIR}/assets
)

set_target_properties(Lighting PROPERTIES
    LINK_FLAGS "/ENTRY:wWinMainCRTStartup"
)
