project(ServiceModuleTests)

enable_language(C CXX ASM)


file(GLOB_RECURSE TEST_SOURCES ${PROJECT_SOURCE_DIR}/Source/*.cpp)
add_executable(ServiceModuleTests EXCLUDE_FROM_ALL ${TEST_SOURCES})

target_include_directories(ServiceModuleTests PRIVATE ${DOCTESTINC})

add_test(NAME ServiceModuleTests COMMAND ServiceModuleTests)

target_link_libraries(ServiceModuleTests PUBLIC
    TactilityKernel
    service-module
    platform-posix
    freertos_kernel
)
