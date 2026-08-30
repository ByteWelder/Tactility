if (NOT DEFINED TACTILITY_SDK_PATH)
    get_filename_component(TACTILITY_SDK_PATH "$ENV{TACTILITY_SDK_PATH}" ABSOLUTE)
endif ()
include("${TACTILITY_SDK_PATH}/TactilitySDK.cmake")

file(GLOB_RECURSE SOURCE_FILES Source/*.c)
tactility_component_register(SRCS ${SOURCE_FILES} INCLUDE_DIRS include)