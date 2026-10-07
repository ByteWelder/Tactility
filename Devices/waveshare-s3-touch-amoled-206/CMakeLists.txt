file(GLOB_RECURSE SOURCE_FILES source/*.c*)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "source"
    REQUIRES TactilityKernel lvgl-module
)

# Board-local link-time hook that pins the LVGL display DPI to the text font (see
# source/lvgl_dpi.cpp). Wrapping lv_display_create() keeps this out of core Tactility/LVGL code.
idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=lv_display_create" APPEND)
