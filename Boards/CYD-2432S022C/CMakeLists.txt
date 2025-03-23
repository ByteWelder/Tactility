file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

idf_component_register(
    SRCS ${SOURCE_FILES} "../../App/Source/TouchCalibrationApp/TouchCalibrationData.cpp"
    INCLUDE_DIRS "Source" "../../App/Source/TouchCalibrationApp"
    REQUIRES Tactility lvgl driver vfs fatfs LovyanGFX PwmBacklight
)
