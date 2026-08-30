file(GLOB_RECURSE SOURCE_FILES Source/*.c)

if (DEFINED ENV{ESP_IDF_VERSION})
    idf_component_register(
        SRCS ${SOURCE_FILES}
        REQUIRES TactilitySDK
            app-module crypt-module gps-module lvgl-module lvgl-window-manager-module service-module
    )
else()
    # A POSIX app is dlopen()ed into a running Tactility process by app-posix-module rather than
    # linked into its own binary - only its headers are needed at compile time, its symbols
    # resolve against Tactility's own copies at load time (see app-posix-module's test fixture).
    # Must be a SHARED object, not a -pie executable: glibc's dlopen() unconditionally refuses
    # any ET_DYN carrying the DF_1_PIE flag ("cannot dynamically load position-independent
    # executable"), regardless of whether it has a dynamic-linker segment - verified empirically.
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${TACTILITY_SDK_PATH}/Modules/app-module/include
        ${TACTILITY_SDK_PATH}/Modules/crypt-module/include
        ${TACTILITY_SDK_PATH}/Modules/gps-module/include
        ${TACTILITY_SDK_PATH}/Modules/lvgl-module/include
        ${TACTILITY_SDK_PATH}/Modules/lvgl-window-manager-module/include
        ${TACTILITY_SDK_PATH}/Modules/service-module/include
        ${TACTILITY_SDK_PATH}/Libraries/TactilityKernel/include
        ${TACTILITY_SDK_PATH}/Libraries/lvgl/include
        ${TACTILITY_SDK_PATH}/Libraries/FreeRTOS-Kernel/include
        ${TACTILITY_SDK_PATH}/Libraries/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix
        ${TACTILITY_SDK_PATH}/Libraries/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix/utils
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
