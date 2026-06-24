add_executable(Light WIN32 main.cpp ${CMAKE_SOURCE_DIR}/icon/app.rc)
target_include_directories(Light PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Light PRIVATE sgkit)

add_custom_command(TARGET Light POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Light>/assets
)
