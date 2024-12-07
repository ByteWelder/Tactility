cmake_minimum_required(VERSION 3.16)

set(TACTILITY_SKIP_SPIFFS 1)
add_definitions(-DESP_TARGET)
add_definitions(-DCONFIG_TT_SPLASH_DURATION=1000)
add_compile_definitions(ESP_TARGET)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

idf_build_set_property(MINIMAL_BUILD ON)

set(EXTRA_COMPONENT_DIRS "../../Libraries/elf_loader")

project(HelloWorld)

include(elf_loader)
project_elf(HelloWorld)

