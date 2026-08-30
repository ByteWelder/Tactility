project(AppPosixModuleTests)

enable_language(C CXX ASM)

# Fixture: a tiny shared object with a main() the test dlopen()s through app-posix-module's real
# loader service. Deliberately not linked against app-module, so its call into
# app_scheduler_current_app_id() stays undefined until dlopen() resolves it against
# AppPosixModuleTests's own copy, exactly like a real Tactility-embedded app would resolve
# against the running Tactility binary.
add_library(app_posix_module_test_fixture SHARED EXCLUDE_FROM_ALL ${CMAKE_CURRENT_LIST_DIR}/fixtures/fixture_app.cpp)
target_include_directories(app_posix_module_test_fixture PRIVATE ${CMAKE_SOURCE_DIR}/Modules/app-module/include)
set_target_properties(app_posix_module_test_fixture PROPERTIES POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/source/*.cpp)
add_executable(AppPosixModuleTests EXCLUDE_FROM_ALL ${TEST_SOURCES})
add_dependencies(AppPosixModuleTests app_posix_module_test_fixture)

target_include_directories(AppPosixModuleTests PRIVATE ${DOCTESTINC})
target_compile_definitions(AppPosixModuleTests PRIVATE
    FIXTURE_APP_PATH="$<TARGET_FILE:app_posix_module_test_fixture>"
)

add_test(NAME AppPosixModuleTests COMMAND AppPosixModuleTests)

target_link_libraries(AppPosixModuleTests PUBLIC
    TactilityKernel
    app-module
    app-posix-module
    service-module
    platform-posix
    freertos_kernel
    ${CMAKE_DL_LIBS}
)
# So the fixture's undefined app_scheduler_current_app_id() reference can resolve against this
# test binary's own copy at dlopen() time. See app-posix-module's own ENABLE_EXPORTS comment on
# the real Tactility executable for why this is needed.
set_target_properties(AppPosixModuleTests PROPERTIES ENABLE_EXPORTS ON)
