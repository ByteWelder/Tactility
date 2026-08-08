cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_LIST_DIR}/../../Buildscripts/module.cmake")

tactility_add_module(esp-epaper-module
    SRCS source/esp_epaper.cpp source/module.cpp
    INCLUDE_DIRS include/
    REQUIRES TactilityKernel platform-esp32 driver esp_epaper
)
