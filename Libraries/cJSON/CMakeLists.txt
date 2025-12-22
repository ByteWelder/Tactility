add_library(cJSON STATIC)
target_sources(cJSON PRIVATE cJSON/cJSON.c cJSON/cJSON_Utils.c)
include_directories(cJSON "cJSON/")
target_include_directories(cJSON PUBLIC "cJSON/")
