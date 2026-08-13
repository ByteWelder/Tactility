project(TactilityKernelTests)

enable_language(C CXX ASM)


file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/source/*.cpp)
add_executable(TactilityKernelTests EXCLUDE_FROM_ALL ${TEST_SOURCES})

target_include_directories(TactilityKernelTests PRIVATE ${DOCTESTINC})

add_test(NAME TactilityKernelTests COMMAND TactilityKernelTests)

target_link_libraries(TactilityKernelTests PUBLIC
    TactilityKernel
    platform-posix
    service-module
)
