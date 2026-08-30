function(tactility_project)
endfunction()

function(_tactility_project)
endfunction()

# POSIX/simulator counterpart to TactilitySDK.esp32.cmake. No elf_loader (ESP32-only ELF
# relocation), no ESP-IDF version check (there's no ESP-IDF, so no idf-version.txt is generated
# for this SDK variant), and no EXTRA_COMPONENT_DIRS/COMPONENTS (those are read only by
# ESP-IDF's own component-discovery build system and have no effect in plain CMake).
macro(tactility_project project_name)
    project(${project_name})
    # The app's own library target is defined in a subdirectory (e.g. "main"); without this it
    # would land nested under that subdirectory instead of directly in the build dir.
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    # ESP-IDF's project() auto-discovers the "main" component; plain CMake doesn't.
    add_subdirectory(main)
endmacro()
