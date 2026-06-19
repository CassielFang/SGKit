# -- Lighting: lighting tutorials (flag + cubes + floor + FPS cam) --
add_executable(Lighting WIN32 main.cpp ${CMAKE_SOURCE_DIR}/icon/app.rc)
target_link_libraries(Lighting PRIVATE sgkit)

add_custom_command(TARGET Lighting POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Lighting>/assets
)
