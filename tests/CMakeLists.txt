# -- Unit tests: each .cpp -> standalone CONSOLE executable
file(GLOB TEST_SOURCES *.cpp)

if(TEST_SOURCES)
    foreach(test_file ${TEST_SOURCES})
        get_filename_component(test_name ${test_file} NAME_WE)
        add_executable(${test_name} ${test_file})
        target_link_libraries(${test_name} PRIVATE sgkit)
        target_compile_definitions(${test_name} PRIVATE _CONSOLE)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
else()
    message(WARNING "No test sources found in tests/")
endif()
