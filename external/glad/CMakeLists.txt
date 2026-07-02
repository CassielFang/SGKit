# -- glad: OpenGL loader
file(GLOB_RECURSE GLAD_SRC "src/*.c")

add_library(glad STATIC ${GLAD_SRC})
target_include_directories(glad PUBLIC "include")
