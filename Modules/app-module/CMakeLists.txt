cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_LIST_DIR}/../../Buildscripts/module.cmake")

file(GLOB_RECURSE SOURCE_FILES "source/*.c*")

tactility_add_module(app-module
    SRCS ${SOURCE_FILES}
    PRIV_INCLUDE_DIRS private/
    INCLUDE_DIRS include/
    REQUIRES TactilityKernel service-module minitar
)

# Tells source/io.cpp its real-syscall fallback must go through __real_read/write/close()
# rather than calling ::read/::write/::close() directly, on every platform whose build wraps
# those symbols (ESP-IDF always; POSIX except macOS, whose linker doesn't support --wrap) -
# see Tactility/CMakeLists.txt and the top-level CMakeLists.txt for where that's applied.
if (NOT APPLE)
    tactility_get_module_name(app-module MODULE_NAME)
    target_compile_definitions(${MODULE_NAME} PRIVATE TT_APP_IO_WRAPS_STDIO)
endif ()
