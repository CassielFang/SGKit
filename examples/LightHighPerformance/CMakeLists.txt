add_executable(LightHighPerformance WIN32 main.cpp ${CMAKE_SOURCE_DIR}/icon/app.rc)
target_include_directories(LightHighPerformance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(LightHighPerformance PRIVATE sgkit)

set_target_properties(LightHighPerformance PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
