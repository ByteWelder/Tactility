project(AppModuleTests)

enable_language(C CXX ASM)


file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/source/*.cpp)
add_executable(AppModuleTests EXCLUDE_FROM_ALL ${TEST_SOURCES})

if (NOT APPLE)
    # Provides __wrap_read/write/close for the -Wl,--wrap= below (see Tactility/CMakeLists.txt
    # for the canonical pairing of this file with those flags).
    target_sources(AppModuleTests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../Tactility/Source/AppStdioWrap.cpp)
endif ()

target_include_directories(AppModuleTests PRIVATE ${DOCTESTINC})

add_test(NAME AppModuleTests COMMAND AppModuleTests)

# Matches app-module's own TT_APP_IO_WRAPS_STDIO (see Modules/app-module/CMakeLists.txt):
# io.cpp calls __real_read/write/close() on non-Apple POSIX, so any final link of it needs
# these wraps too, or those symbols go unresolved. The printf-family/getc-family flags are
# needed for the same reason: AppStdioWrap.cpp compiles those __wrap_* functions unconditionally
# on this platform (see its own #if guard), and they reference __real_vfprintf/fputs/fputc/
# fgetc/fgets - those only exist once the matching --wrap flag is passed.
if (NOT APPLE)
    target_link_options(AppModuleTests PRIVATE
        "-Wl,--wrap=read" "-Wl,--wrap=write" "-Wl,--wrap=close"
        "-Wl,--wrap=printf" "-Wl,--wrap=fprintf" "-Wl,--wrap=vprintf" "-Wl,--wrap=vfprintf"
        "-Wl,--wrap=puts" "-Wl,--wrap=fputs" "-Wl,--wrap=putchar" "-Wl,--wrap=fputc"
        "-Wl,--wrap=getchar" "-Wl,--wrap=fgetc" "-Wl,--wrap=fgets"
    )
endif ()

target_link_libraries(AppModuleTests PUBLIC
    TactilityKernel
    app-module
    service-module
    platform-posix
    freertos_kernel
)
