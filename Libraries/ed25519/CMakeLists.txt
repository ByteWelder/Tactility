cmake_minimum_required(VERSION 3.20)

if (DEFINED ENV{ESP_IDF_VERSION})
    idf_component_register(
        SRC_DIRS "Source/"
        INCLUDE_DIRS "Include/"
    )
    target_compile_definitions(${COMPONENT_LIB} PUBLIC ED25519_NO_SEED)
else()
    file(GLOB SOURCES "Source/*.c")

    add_library(ed25519 STATIC)

    target_sources(ed25519 PRIVATE ${SOURCES})
    target_include_directories(ed25519 PUBLIC "Include/")
    target_compile_definitions(ed25519 PUBLIC ED25519_NO_SEED)
endif()
