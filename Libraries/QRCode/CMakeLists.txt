cmake_minimum_required(VERSION 3.20)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES src/*.c)

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "src/"
    )

    add_definitions(-DESP_PLATFORM)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-unknown-pragmas)
    endif()
else()
    file(GLOB SOURCES "src/*.c")
    file(GLOB HEADERS "src/*.h")

    add_library(QRCode STATIC)

    target_sources(QRCode
        PRIVATE ${SOURCES}
        PUBLIC ${HEADERS}
    )

    target_include_directories(QRCode
        PUBLIC src
    )
endif()
