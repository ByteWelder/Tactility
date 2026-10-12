project(MeshMeshtasticModuleTests)

enable_language(C CXX ASM)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/source/*.cpp)
add_executable(MeshMeshtasticModuleTests EXCLUDE_FROM_ALL ${TEST_SOURCES})

target_include_directories(MeshMeshtasticModuleTests PRIVATE
    ${DOCTESTINC}
    ${PROJECT_SOURCE_DIR}/../private
)

add_test(NAME MeshMeshtasticModuleTests COMMAND MeshMeshtasticModuleTests)

target_link_libraries(MeshMeshtasticModuleTests PUBLIC
    mesh-meshtastic-module
    platform-posix
)
