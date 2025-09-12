cmake_minimum_required(VERSION 3.20)

set(C_STANDARD 11)
set(CC_STANDARD_REQUIRED ON)

if (DEFINED ENV{ESP_IDF_VERSION})
    idf_component_register(
        SRC_DIRS "minitar/src/"
        INCLUDE_DIRS "minitar/"
    )

else()
    file(GLOB SOURCES "minitar/src/*.*")
    file(GLOB HEADERS "minitar/*.h")

    add_library(minitar STATIC)

    target_sources(minitar PRIVATE ${SOURCES})
    include_directories(minitar "minitar/src/")
    target_include_directories(minitar PUBLIC "minitar/")

endif()
