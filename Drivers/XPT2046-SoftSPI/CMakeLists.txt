idf_component_register(
    SRCS "XPT2046_TouchscreenSOFTSPI.cpp"
    INCLUDE_DIRS "."
    REQUIRES Tactility driver esp_timer
)
