file(GLOB SOURCES "cJSON/*.c")

# We ned to wrap the official CMakeLists.txt, because it otherwise breaks the builds for Linux and macOS (linker errors)
add_library(cJSON STATIC)
target_sources(cJSON PRIVATE ${SOURCES})
include_directories(cJSON "cJSON/")
target_include_directories(cJSON PUBLIC "cJSON/")
