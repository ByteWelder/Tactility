// SPDX-License-Identifier: Apache-2.0
#include <location/location_settings.h>

#include <service/paths.h>

#include <tactility/filesystem/fs.h>
#include <tactility/log.h>
#include <tactility/paths.h>
#include <tactility/properties_file.h>

#include <cstring>

constexpr auto* TAG = "location_settings";
constexpr auto* SETTINGS_FILE_NAME = "settings.properties";
constexpr auto* KEY_ENABLED = "enabled";
constexpr auto* KEY_PRECISION = "precision";
constexpr auto* PRECISION_APPROXIMATE = "approximate";
constexpr auto* PRECISION_PRECISE = "precise";

void location_settings_load(LocationSettings* settings) {
    *settings = LocationSettings {
        .enabled = false,
        .precision = LOCATION_PRECISION_PRECISE,
    };

    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (service_paths_get_user_data_path(LOCATION_SERVICE_ID, SETTINGS_FILE_NAME, path, sizeof(path)) != ERROR_NONE) {
        return;
    }

    PropertiesFile* file = properties_file_open(path);
    if (file == nullptr) {
        return;
    }

    char value[32];
    if (properties_file_get(file, KEY_ENABLED, value, sizeof(value)) == ERROR_NONE) {
        settings->enabled = strcmp(value, "true") == 0;
    }
    if (properties_file_get(file, KEY_PRECISION, value, sizeof(value)) == ERROR_NONE) {
        settings->precision = strcmp(value, PRECISION_APPROXIMATE) == 0 ? LOCATION_PRECISION_APPROXIMATE : LOCATION_PRECISION_PRECISE;
    }
    properties_file_close(file);
}

error_t location_settings_save(const LocationSettings* settings) {
    char directory[FILE_MAX_PATH_STRING_LENGTH];
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (service_paths_get_user_data_directory(LOCATION_SERVICE_ID, directory, sizeof(directory)) != ERROR_NONE ||
        service_paths_get_user_data_path(LOCATION_SERVICE_ID, SETTINGS_FILE_NAME, path, sizeof(path)) != ERROR_NONE) {
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
    properties_file_set(file, KEY_PRECISION, settings->precision == LOCATION_PRECISION_APPROXIMATE ? PRECISION_APPROXIMATE : PRECISION_PRECISE);
    return properties_file_close(file);
}
