cmake_minimum_required(VERSION 3.20)

if (NOT DEFINED ENV{TACTILITY_SDK_PATH})
    message(FATAL_ERROR "TACTILITY_SDK_PATH environment variable is not set")
endif()

# Absolute, since it's later used (e.g. main/CMakeLists.txt's target_include_directories) from
# subdirectories where a relative path would resolve against the wrong directory.
get_filename_component(TACTILITY_SDK_PATH "$ENV{TACTILITY_SDK_PATH}" ABSOLUTE)
include("${TACTILITY_SDK_PATH}/TactilitySDK.cmake")
tactility_project(SdkTest)
