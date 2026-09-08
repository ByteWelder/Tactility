cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_LIST_DIR}/../../Buildscripts/module.cmake")

file(GLOB_RECURSE SOURCE_FILES "source/*.c*")

tactility_add_module(app-module
    SRCS ${SOURCE_FILES}
    PRIV_INCLUDE_DIRS private/
    INCLUDE_DIRS include/
    REQUIRES TactilityKernel service-module minitar
    PRIV_REQUIRES TactilityKernelCpp
)

# Tells source/io.cpp its real-syscall fallback must go through __real_read/write/close()
# rather than calling ::read/::write/::close() directly, on every platform whose build wraps
# those symbols (ESP-IDF always; POSIX except macOS, whose linker doesn't support --wrap) -
# see Tactility/CMakeLists.txt and the top-level CMakeLists.txt for where that's applied.
if (NOT APPLE)
    tactility_get_module_name(app-module MODULE_NAME)
    target_compile_definitions(${MODULE_NAME} PRIVATE TT_APP_IO_WRAPS_STDIO)
endif ()

# install.cpp resolves an installed binary's fixed bin/<platform>/<binary>.so path itself, so it
# needs the same platform-arch string app-posix-module's own CMakeLists.txt defines for its
# loader (the ESP32 equivalent, CONFIG_IDF_TARGET, comes for free from sdkconfig.h there).
if (NOT ESP_PLATFORM)
    tactility_get_module_name(app-module MODULE_NAME)
    target_compile_definitions(${MODULE_NAME} PRIVATE "TACTILITY_POSIX_ARCH=\"${CMAKE_SYSTEM_PROCESSOR}\"")
endif ()
