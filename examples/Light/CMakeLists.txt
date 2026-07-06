add_executable(Light WIN32 main.cpp objects.cpp ${CMAKE_SOURCE_DIR}/icon/app.rc)
target_include_directories(Light PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Light PRIVATE sgkit)

#set_target_properties(Light PROPERTIES
#    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#)
add_custom_command(TARGET Light POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/external/assimp/bin/assimp-vc145-mt.dll
        $<TARGET_FILE_DIR:Light>
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Light>/assets
)
