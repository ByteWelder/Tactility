target_compile_definitions(lvgl PUBLIC "-DLV_CONF_INCLUDE_SIMPLE")
target_include_directories(lvgl PUBLIC ${LVGL_ROOT_DIR}/config)
