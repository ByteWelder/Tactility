cmake_minimum_required(VERSION 3.20)

if (DEFINED ENV{ESP_IDF_VERSION})

    idf_component_register(
        INCLUDE_DIRS "Include/"
        REQUIRES TactilityKernel
    )

else()

    add_library(TactilityKernelCpp INTERFACE)

    target_include_directories(TactilityKernelCpp
        INTERFACE Include/
    )

    target_link_libraries(TactilityKernelCpp
        INTERFACE TactilityKernel
    )
endif()
