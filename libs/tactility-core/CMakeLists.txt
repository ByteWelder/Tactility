cmake_minimum_required(VERSION 3.16)
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "src/*.h")

add_library(tactility-core OBJECT)
target_sources(tactility-core
    PRIVATE ${SOURCES}
    PUBLIC ${HEADERS}
)
target_include_directories(tactility-core
    PRIVATE src/
    INTERFACE src/
)

if (ESP_PLATFORM)
    add_definitions(-DESP_PLATFORM)
    target_link_libraries(tactility-core
        PUBLIC mlib
        PUBLIC idf::freertos
        PRIVATE idf::mbedtls
        PRIVATE idf::nvs_flash
        PRIVATE idf::esp_hw_support
    )
else()
    target_link_libraries(tactility-core PRIVATE mlib)
    target_link_libraries(tactility-core PRIVATE mbedtls)
    target_link_libraries(tactility-core PRIVATE freertos)
endif()
