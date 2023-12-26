cmake_minimum_required(VERSION 3.16)

add_definitions(-DFURI_DEBUG)

set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(NanoBake)
