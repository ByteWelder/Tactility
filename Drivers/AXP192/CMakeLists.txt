file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Include"
    PRIV_INCLUDE_DIRS "Private"
    REQUIRES Tactility
)
