cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "src/*.h")

add_library(tactility-core OBJECT)
target_sources(tactility-core
    PRIVATE ${SOURCES}
    PUBLIC ${HEADERS}
)
target_include_directories(tactility-core SYSTEM
    PRIVATE src/
    INTERFACE src/
)

if (DEFINED ENV{ESP_IDF_VERSION})
    add_definitions(-DESP_PLATFORM)
    target_link_libraries(tactility-core
        PUBLIC mlib
        PUBLIC idf::mbedtls
        PRIVATE idf::nvs_flash # ESP-IDF // for secure.c
    )
else()
    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
    target_link_libraries(tactility-core
    	PUBLIC mlib
    	PUBLIC mbedtls
    	PUBLIC freertos-kernel
    )
endif()
