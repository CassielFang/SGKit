# -- glad: OpenGL loader -------------------------------------------------
# OBJECT library -- .obj files are linked directly into sgkit.lib,
add_library(glad OBJECT src/glad.c)
target_include_directories(glad PRIVATE include)   # hidden from consumers
