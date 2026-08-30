function(tactility_project)
endfunction()

function(_tactility_project)
endfunction()

macro(tactility_project_pre project_name)
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    set(EXTRA_COMPONENT_DIRS ${TACTILITY_SDK_PATH} ${TACTILITY_SDK_PATH}/Modules)
endmacro()

macro(tactility_project_post project_name)
    set(TACTILITY_SKIP_SPIFFS 1)

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

macro(tactility_component_register)
    cmake_parse_arguments(TT_COMPONENT "" "" "SRCS;INCLUDE_DIRS;REQUIRES;PRIV_REQUIRES" ${ARGN})
    idf_component_register(
        SRCS ${TT_COMPONENT_SRCS}
        INCLUDE_DIRS ${TT_COMPONENT_INCLUDE_DIRS}
        REQUIRES TactilitySDK ${TT_COMPONENT_REQUIRES}
        PRIV_REQUIRES ${TT_COMPONENT_PRIV_REQUIRES}
    )
endmacro()
