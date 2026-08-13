project(tests)

set(DOCTESTINC ${PROJECT_SOURCE_DIR}/Doctest/Include)

enable_testing()
add_subdirectory(${CMAKE_SOURCE_DIR}/Modules/service-module/tests ${CMAKE_CURRENT_BINARY_DIR}/service-module)
add_subdirectory(${CMAKE_SOURCE_DIR}/TactilityFreeRtos/Tests ${CMAKE_CURRENT_BINARY_DIR}/TactilityFreeRtos)
add_subdirectory(${CMAKE_SOURCE_DIR}/TactilityKernel/tests ${CMAKE_CURRENT_BINARY_DIR}/TactilityKernel)
add_subdirectory(${CMAKE_SOURCE_DIR}/Tactility/Tests ${CMAKE_CURRENT_BINARY_DIR}/Tactility)
add_subdirectory(${CMAKE_SOURCE_DIR}/Modules/crypt-module/tests ${CMAKE_CURRENT_BINARY_DIR}/crypt-module)
add_subdirectory(${CMAKE_SOURCE_DIR}/Modules/app-module/tests ${CMAKE_CURRENT_BINARY_DIR}/app-module)

add_custom_target(build-tests)
add_dependencies(build-tests ServiceModuleTests)
add_dependencies(build-tests TactilityFreeRtosTests)
add_dependencies(build-tests TactilityTests)
add_dependencies(build-tests TactilityKernelTests)
add_dependencies(build-tests CryptModuleTests)
add_dependencies(build-tests AppModuleTests)
