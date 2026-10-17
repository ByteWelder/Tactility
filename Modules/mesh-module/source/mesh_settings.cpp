// SPDX-License-Identifier: Apache-2.0
#include <mesh/mesh_settings.h>

#include <service/paths.h>

#include <tactility/filesystem/fs.h>
#include <tactility/log.h>
#include <tactility/paths.h>
#include <tactility/properties_file.h>

#include <cstdio>
#include <cstring>

#ifdef ESP_PLATFORM
#include <esp_random.h>
#else
#include <random>
#endif

constexpr auto* TAG = "mesh_settings";
constexpr auto* SETTINGS_FILE_NAME = "settings.properties";
constexpr auto* KEY_ENABLED = "enabled";
constexpr auto* KEY_BACKEND = "backend";
constexpr auto* KEY_PRESET = "preset";
constexpr auto* KEY_NODE_NAME = "nodeName";
constexpr auto* KEY_RADIO = "radio";
// Earlier versions stored this as the name when the user hadn't chosen one
constexpr auto* LEGACY_DEFAULT_NODE_NAME = "Tactility";

static uint32_t random_u32() {
#ifdef ESP_PLATFORM
    return esp_random();
#else
    std::random_device device;
    return device();
#endif
}

static void read_settings_file(MeshSettings* settings) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (service_paths_get_user_data_path(MESH_SERVICE_ID, SETTINGS_FILE_NAME, path, sizeof(path)) != ERROR_NONE) {
        return;
    }

    PropertiesFile* file = properties_file_open(path);
    if (file == nullptr) {
        return;
    }

    char value[64];
    if (properties_file_get(file, KEY_ENABLED, value, sizeof(value)) == ERROR_NONE) {
        settings->enabled = strcmp(value, "true") == 0;
    }
    if (properties_file_get(file, KEY_BACKEND, value, sizeof(value)) == ERROR_NONE) {
        strncpy(settings->backend, value, MESH_BACKEND_ID_MAX_LENGTH);
    }
    if (properties_file_get(file, KEY_PRESET, value, sizeof(value)) == ERROR_NONE) {
        strncpy(settings->preset, value, MESH_NAME_MAX_LENGTH);
    }
    if (properties_file_get(file, KEY_NODE_NAME, value, sizeof(value)) == ERROR_NONE && value[0] != '\0') {
        strncpy(settings->node_name, value, MESH_NAME_MAX_LENGTH);
    }
    if (properties_file_get(file, KEY_RADIO, value, sizeof(value)) == ERROR_NONE) {
        strncpy(settings->radio, value, MESH_NAME_MAX_LENGTH);
    }
    properties_file_close(file);
}

void mesh_settings_load(MeshSettings* settings) {
    *settings = {};
    read_settings_file(settings);

    // A name that the user didn't choose is made up once and then kept
    if (settings->node_name[0] == '\0' || strcmp(settings->node_name, LEGACY_DEFAULT_NODE_NAME) == 0) {
        snprintf(settings->node_name, sizeof(settings->node_name), "TactilityUser-%08x", (unsigned)random_u32());
        if (mesh_settings_save(settings) != ERROR_NONE) {
            LOG_E(TAG, "Failed to save the generated name");
        }
    }
}

error_t mesh_settings_save(const MeshSettings* settings) {
    char directory[FILE_MAX_PATH_STRING_LENGTH];
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (service_paths_get_user_data_directory(MESH_SERVICE_ID, directory, sizeof(directory)) != ERROR_NONE ||
        service_paths_get_user_data_path(MESH_SERVICE_ID, SETTINGS_FILE_NAME, path, sizeof(path)) != ERROR_NONE) {
        return ERROR_BUFFER_OVERFLOW;
    }
    const error_t made = directory_make(directory, true);
    if (made != ERROR_NONE && made != ERROR_ALREADY_EXISTS) {
        LOG_E(TAG, "Failed to create %s", directory);
        return ERROR_RESOURCE;
    }

    PropertiesFile* file = properties_file_open(path);
    if (file == nullptr) {
        return ERROR_RESOURCE;
    }
    properties_file_set(file, KEY_ENABLED, settings->enabled ? "true" : "false");
    properties_file_set(file, KEY_BACKEND, settings->backend);
    properties_file_set(file, KEY_PRESET, settings->preset);
    properties_file_set(file, KEY_NODE_NAME, settings->node_name);
    properties_file_set(file, KEY_RADIO, settings->radio);
    return properties_file_close(file);
}
