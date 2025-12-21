file(GLOB_RECURSE SOURCE_FILES Source/*.c* Include/*.hpp)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Include" "Source"
    REQUIRES Tactility LovyanGFX lvgl
)
