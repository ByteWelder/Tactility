file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Source"
    REQUIRES Tactility EspLcdCompat esp_lcd_st7701 esp_lcd_panel_io_additions GT911 PwmBacklight driver vfs fatfs
)
