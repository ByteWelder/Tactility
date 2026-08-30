function(tactility_project)
endfunction()

function(_tactility_project)
endfunction()

macro(tactility_project_pre project_name)
endmacro()

macro(tactility_project_post project_name)
    # The app's own library target is defined in a subdirectory (e.g. "main"); without this it
    # would land nested under that subdirectory instead of directly in the build dir.
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    # ESP-IDF's project() auto-discovers the "main" component; plain CMake doesn't.
    add_subdirectory(main)
endmacro()
