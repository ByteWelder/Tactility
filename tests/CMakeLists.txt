project(tests)

set(DOCTESTINC ${PROJECT_SOURCE_DIR}/include)

enable_testing()
add_subdirectory(tactility-core)

add_custom_target(build-tests)
add_dependencies(build-tests tactility-core-tests)
