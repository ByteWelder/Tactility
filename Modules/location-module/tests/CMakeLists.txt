project(LocationModuleTests)

enable_language(C CXX ASM)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/source/*.cpp)
add_executable(LocationModuleTests EXCLUDE_FROM_ALL ${TEST_SOURCES})

target_include_directories(LocationModuleTests PRIVATE
    ${DOCTESTINC}
    ${PROJECT_SOURCE_DIR}/../private
)

add_test(NAME LocationModuleTests COMMAND LocationModuleTests)

target_link_libraries(LocationModuleTests PUBLIC
    TactilityKernel
    platform-posix
    service-module
    location-module
)
