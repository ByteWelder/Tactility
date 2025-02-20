idf_component_register(
    SRC_DIRS "Source" "Source/hal"
    INCLUDE_DIRS "Source"
    REQUIRES Tactility esp_lvgl_port ILI934x FT5x06 AXP2101 AW9523 driver vfs fatfs
)
