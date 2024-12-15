cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "Source/"
        REQUIRES lvgl
        PRIV_REQUIRES Tactility TactilityCore TactilityHeadless elf_loader
    )

    add_definitions(-DESP_PLATFORM)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-unused-variable)
    endif()
else()
    file(GLOB_RECURSE SOURCES "Source/*.c**")
    file(GLOB_RECURSE HEADERS "Source/*.h**")

    add_library(TactilityC OBJECT)

    target_sources(TactilityC
        PRIVATE ${SOURCES}
        PUBLIC ${HEADERS}
    )

    target_include_directories(TactilityC
        PUBLIC Source/
    )

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
    target_link_libraries(TactilityC
        PRIVATE Tactility
        PRIVATE TactilityCore
        PRIVATE TactilityHeadless
        PUBLIC lvgl
    )
endif()

