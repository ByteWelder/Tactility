file(GLOB_RECURSE SOURCE_FILES Source/*.c)

idf_component_register(
    SRCS ${SOURCE_FILES}
    REQUIRES TactilitySDK
        app-module crypt-module gps-module lvgl-module lvgl-window-manager-module service-module
)
