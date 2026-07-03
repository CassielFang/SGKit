# -- std_image
file(GLOB_RECURSE STB_SRC "src/*.c")

add_library(stb STATIC ${STB_SRC})
target_include_directories(stb PUBLIC "include")
