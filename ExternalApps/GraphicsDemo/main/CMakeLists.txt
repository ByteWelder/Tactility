file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

idf_component_register(
    SRC_DIRS "Source"
    INCLUDE_DIRS "Include"
    REQUIRES TactilitySDK
)
