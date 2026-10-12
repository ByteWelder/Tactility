project(MeshModuleTests)

enable_language(C CXX ASM)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/source/*.cpp)
add_executable(MeshModuleTests EXCLUDE_FROM_ALL ${TEST_SOURCES})

target_include_directories(MeshModuleTests PRIVATE
    ${DOCTESTINC}
    ${PROJECT_SOURCE_DIR}/../private
)

add_test(NAME MeshModuleTests COMMAND MeshModuleTests)

target_link_libraries(MeshModuleTests PUBLIC
    TactilityKernel
    platform-posix
    service-module
    mesh-module
    mock-drivers-module
)
