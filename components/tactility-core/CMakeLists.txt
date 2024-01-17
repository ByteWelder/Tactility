idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "src"
    REQUIRES mbedtls esp_hw_support nvs_flash
)

target_link_libraries(${COMPONENT_LIB} ${IDF_TARGET_NAME} mlib)

