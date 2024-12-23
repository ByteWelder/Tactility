cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "Source/"
        PRIV_INCLUDE_DIRS "Private/"
        REQUIRES TactilityHeadless lvgl driver elf_loader lv_screenshot QRCode
    )

    add_definitions(-DESP_PLATFORM)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-unused-variable)
    endif()
else()
    file(GLOB_RECURSE SOURCES "Source/*.c*")
    file(GLOB_RECURSE HEADERS "Source/*.h*")

    add_library(Tactility OBJECT)

    target_sources(Tactility
        PRIVATE ${SOURCES}
        PUBLIC ${HEADERS}
    )

    include_directories(
        PRIVATE Private/
    )

    target_include_directories(Tactility
        PUBLIC Source/
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

