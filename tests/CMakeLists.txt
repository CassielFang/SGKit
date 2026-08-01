# Unit tests

# Auto-discover test sources
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(TEST_SRC ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE sgkit)
    target_sources(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/icon/app.rc)
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/icon)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
