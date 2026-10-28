option(ASSERTS "Keep assertions enabled in release builds" OFF)
if(ASSERTS)
    add_compile_options(-UNDEBUG)
endif()
