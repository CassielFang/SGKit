add_executable(Light WIN32 main.cpp ${CMAKE_SOURCE_DIR}/icon/app.rc)
target_include_directories(Light PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Light PRIVATE sgkit)

set_target_properties(Light PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
