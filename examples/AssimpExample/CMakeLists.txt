add_executable(AssimpExample WIN32 main.cpp ${CMAKE_SOURCE_DIR}/icon/app.rc)
target_include_directories(AssimpExample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(AssimpExample PRIVATE sgkit)

add_custom_command(TARGET AssimpExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/external/assimp/bin/assimp-vc145-mt.dll
        $<TARGET_FILE_DIR:AssimpExample>
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:AssimpExample>/assets
)
