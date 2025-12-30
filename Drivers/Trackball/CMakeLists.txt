cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (DEFINED ENV{ESP_IDF_VERSION})
    idf_component_register(
        SRC_DIRS "Source/"
        INCLUDE_DIRS "Source/"
        REQUIRES TactilityCore driver lvgl
    )

else()
    file(GLOB SOURCES "Source/*.c*")
    file(GLOB HEADERS "Source/*.h*")

    add_library(Trackball STATIC)

    target_sources(Trackball PRIVATE ${SOURCES})
    include_directories(Trackball "Source/")
    target_include_directories(Trackball PUBLIC "Source/")

endif()
