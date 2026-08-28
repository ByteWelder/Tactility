project(HttpModuleTests)

enable_language(C CXX ASM)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/source/*.cpp)
add_executable(HttpModuleTests EXCLUDE_FROM_ALL ${TEST_SOURCES})

target_include_directories(HttpModuleTests PRIVATE ${DOCTESTINC})

add_test(NAME HttpModuleTests COMMAND HttpModuleTests)

target_link_libraries(HttpModuleTests PUBLIC
    http-module
    TactilityKernel
    freertos_kernel
)
