# -- Individual examples
add_subdirectory(Light)
add_subdirectory(AssimpExample)

if(SGK_ENABLE_SCRIPTING)
    add_subdirectory(ScriptExample)
endif()
