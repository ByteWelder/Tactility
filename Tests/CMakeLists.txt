project(tests)

set(DOCTESTINC ${PROJECT_SOURCE_DIR}/Include)

enable_testing()
add_subdirectory(TactilityCore)
add_subdirectory(Tactility)

add_custom_target(build-tests)
add_dependencies(build-tests TactilityCoreTests)
add_dependencies(build-tests TactilityTests)
