cmake_minimum_required(VERSION 3.20)

set(TACTILITY_SKIP_SPIFFS 1)
add_definitions(-DESP_TARGET)
add_compile_definitions(ESP_TARGET)

add_definitions(-DLV_CONF_PATH=../../../Boards/Simulator/Source/lv_conf.h)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "../../Libraries/elf_loader")

project(HelloWorld)

include(elf_loader)
project_elf(HelloWorld)

