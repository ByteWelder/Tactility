project(tactility-core-tests)

enable_language(C CXX ASM)

set(CMAKE_CXX_COMPILER g++)

file(GLOB_RECURSE TEST_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(tactility-core-tests EXCLUDE_FROM_ALL ${TEST_SOURCES})

target_include_directories(tactility-core-tests PRIVATE
    ${DOCTESTINC}
)

add_test(NAME tactility-core-tests
    COMMAND tactility-core-tests
)

target_link_libraries(tactility-core-tests
    PUBLIC tactility-core
        freertos-kernel
)
