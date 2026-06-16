# Platform detection and system libraries

if(WIN32)
    set(SGK_PLATFORM_LIBS gdi32 user32 opengl32 imm32)
elseif(APPLE)
    set(SGK_PLATFORM_LIBS "-framework Cocoa -framework OpenGL")
else()
    set(SGK_PLATFORM_LIBS X11 GL)
endif()
