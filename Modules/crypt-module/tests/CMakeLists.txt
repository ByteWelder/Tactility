project(CryptModuleTests)

enable_language(C CXX ASM)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/source/*.cpp)
add_executable(CryptModuleTests EXCLUDE_FROM_ALL ${TEST_SOURCES})

target_include_directories(CryptModuleTests PRIVATE ${DOCTESTINC})

add_test(NAME CryptModuleTests COMMAND CryptModuleTests)

target_link_libraries(CryptModuleTests PUBLIC
    crypt-module
    TactilityKernel
    freertos_kernel
)
