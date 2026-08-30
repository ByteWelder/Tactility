function(tactility_project)
endfunction()

function(_tactility_project)
endfunction()

macro(tactility_project project_name)
    set(TACTILITY_SKIP_SPIFFS 1)

    # Tactility's PanicHandler.cpp needs s0 to stay a frame pointer to capture a callstack for
    # a RISC-V app's crashes, which GCC does not guarantee without this flag. The firmware sets the
    # same flag for its own code, but a crash usually happens in app code, built separately here.
    # Gated to RISC-V since Xtensa never reads s0 this way. idf_build_set_property(), not
    # add_compile_options(): the app's code compiles as an idf_component_register() component
    # (Apps/*/main/CMakeLists.txt), which reads ESP-IDF's own COMPILE_OPTIONS build property rather
    # than plain directory-scoped flags. project_elf() below uses the same property for its own
    # flags for the same reason.
    if(CONFIG_IDF_TARGET_ARCH_RISCV)
        idf_build_set_property(COMPILE_OPTIONS "-fno-omit-frame-pointer" APPEND)
    endif()

    include("${TACTILITY_SDK_PATH}/Libraries/elf_loader/elf_loader.cmake")
    project_elf($project_name)

    file(READ ${TACTILITY_SDK_PATH}/idf-version.txt TACTILITY_SDK_IDF_VERSION)
    string(REGEX REPLACE "^([0-9]+\\.[0-9]+).*" "\\1" TACTILITY_SDK_IDF_MAJOR_MINOR "${TACTILITY_SDK_IDF_VERSION}")
    string(REGEX REPLACE "^([0-9]+\\.[0-9]+).*" "\\1" CURRENT_IDF_MAJOR_MINOR "$ENV{ESP_IDF_VERSION}")
    if (NOT "${CURRENT_IDF_MAJOR_MINOR}" STREQUAL "${TACTILITY_SDK_IDF_MAJOR_MINOR}")
        message(FATAL_ERROR "ESP-IDF version of Tactility SDK (${TACTILITY_SDK_IDF_VERSION}) does not match current ESP-IDF version ($ENV{ESP_IDF_VERSION})")
    endif()

    set(EXTRA_COMPONENT_DIRS
        "${TACTILITY_SDK_PATH}/Libraries/TactilityFreeRtos"
        "${TACTILITY_SDK_PATH}/Modules"
    )

    set(COMPONENTS
        TactilityFreeRtos
        app-module
        crypt-module
        gps-module
        lvgl-module
        lvgl-window-manager-module
        service-module
    )

endmacro()
