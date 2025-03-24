file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Source"
    REQUIRES Tactility lvgl esp_lvgl_port esp_lcd driver vfs fatfs CST816S PwmBacklight 
)
