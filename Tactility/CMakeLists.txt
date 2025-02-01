cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "Include/"
        PRIV_INCLUDE_DIRS "Private/"
        REQUIRES TactilityHeadless lvgl driver elf_loader lv_screenshot QRCode
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-unused-variable)
    endif()
else()
    file(GLOB_RECURSE SOURCES "Source/*.c*")

    add_library(Tactility OBJECT)

    target_sources(Tactility
        PRIVATE ${SOURCES}
    )

    include_directories(
        PRIVATE Private/
    )

    target_include_directories(Tactility
        PUBLIC Include/
    )

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)

    target_link_libraries(Tactility
        PUBLIC TactilityHeadless
        PUBLIC lvgl
        PUBLIC freertos_kernel
        PUBLIC lv_screenshot
    )
endif()

