file(GLOB SOURCES "cJSON/*.c")

add_library(cJSON STATIC)
target_sources(cJSON PRIVATE ${SOURCES})
include_directories(cJSON "cJSON/")
target_include_directories(cJSON PUBLIC "cJSON/")
