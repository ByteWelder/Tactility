cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "Include/"
        REQUIRES lvgl
        PRIV_REQUIRES Tactility TactilityCore elf_loader
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-unused-variable)
    endif()
else()
    file(GLOB_RECURSE SOURCES "Source/*.c**")

    add_library(TactilityC OBJECT)

    target_sources(TactilityC
        PRIVATE ${SOURCES}
    )

    target_include_directories(TactilityC
        PUBLIC Include/
    )

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
    target_link_libraries(TactilityC
        PRIVATE Tactility
        PRIVATE TactilityCore
        PUBLIC lvgl
    )
endif()

