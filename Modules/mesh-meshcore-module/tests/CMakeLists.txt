project(MeshMeshCoreModuleTests)

enable_language(C CXX ASM)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/source/*.cpp)
add_executable(MeshMeshCoreModuleTests EXCLUDE_FROM_ALL ${TEST_SOURCES})

target_include_directories(MeshMeshCoreModuleTests PRIVATE
    ${DOCTESTINC}
    ${PROJECT_SOURCE_DIR}/../private
)

add_test(NAME MeshMeshCoreModuleTests COMMAND MeshMeshCoreModuleTests)

target_link_libraries(MeshMeshCoreModuleTests PUBLIC
    mesh-meshcore-module
    platform-posix
)
