# Yellow Board only runs on ESP32
set(project_components nanobake)

if("${IDF_TARGET}" STREQUAL "esp32")
    list(APPEND project_components yellow_board)
endif()

# T-Deck is an S3 platform
if("${IDF_TARGET}" STREQUAL "esp32s3")
    list(APPEND project_components lilygo_tdeck)
endif()

idf_component_register(
    SRC_DIRS "src"
        "src/hello_world"
    REQUIRES ${project_components}
)
