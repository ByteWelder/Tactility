cmake_minimum_required(VERSION 3.16)

set(BOARD_COMPONENTS esp_wifi)

file(GLOB_RECURSE SOURCE_FILES src/*.c)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "src/"
    REQUIRES esp_wifi nvs_flash spiffs
)

set(ASSETS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../data/assets")
spiffs_create_partition_image(assets ${ASSETS_SRC_DIR} FLASH_IN_PROJECT)

set(CONFIG_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../data/config")
spiffs_create_partition_image(config ${CONFIG_SRC_DIR} FLASH_IN_PROJECT)

target_link_libraries(${COMPONENT_LIB} ${IDF_TARGET_NAME} tactility)
target_link_libraries(${COMPONENT_LIB} ${IDF_TARGET_NAME} tactility-core)

