// SPDX-License-Identifier: Apache-2.0
#include <tactility/radio_settings.h>

#include <tactility/device.h>
#include <tactility/filesystem/fs.h>
#include <tactility/log.h>
#include <tactility/paths.h>
#include <tactility/properties_file.h>

#include <cstdio>

constexpr auto* TAG = "radio_settings";
constexpr auto* KEY_REGION = "region";

static error_t get_settings_paths(char* directory, size_t directory_size, char* path, size_t path_size) {
    char data_path[FILE_MAX_PATH_STRING_LENGTH];
    const error_t error = paths_get_data_path(data_path, sizeof(data_path));
    if (error != ERROR_NONE) {
        return error;
    }
    const int directory_length = snprintf(directory, directory_size, "%s/settings", data_path);
    const int path_length = snprintf(path, path_size, "%s/radio.properties", directory);
    if (directory_length < 0 || static_cast<size_t>(directory_length) >= directory_size ||
        path_length < 0 || static_cast<size_t>(path_length) >= path_size) {
        return ERROR_BUFFER_OVERFLOW;
    }
    return ERROR_NONE;
}

static void apply_to_devices(RadioRegion region) {
    device_for_each_of_type(&RADIO_TYPE, &region, [](Device* device, void* context) {
        radio_set_region(device, *static_cast<RadioRegion*>(context));
        return true;
    });
}

extern "C" {

RadioRegion radio_settings_load_region() {
    char directory[FILE_MAX_PATH_STRING_LENGTH];
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (get_settings_paths(directory, sizeof(directory), path, sizeof(path)) != ERROR_NONE) {
        return RADIO_REGION_UNSET;
    }

    PropertiesFile* file = properties_file_open(path);
    if (file == nullptr) {
        return RADIO_REGION_UNSET;
    }
    RadioRegion region = RADIO_REGION_UNSET;
    char value[32];
    if (properties_file_get(file, KEY_REGION, value, sizeof(value)) == ERROR_NONE &&
        radio_region_from_string(value, &region) != ERROR_NONE) {
        LOG_W(TAG, "Unknown region \"%s\"", value);
    }
    properties_file_close(file);
    return region;
}

error_t radio_settings_save_region(RadioRegion region) {
    char directory[FILE_MAX_PATH_STRING_LENGTH];
    char path[FILE_MAX_PATH_STRING_LENGTH];
    error_t error = get_settings_paths(directory, sizeof(directory), path, sizeof(path));
    if (error != ERROR_NONE) {
        return error;
    }
    error = directory_make(directory, true);
    if (error != ERROR_NONE && error != ERROR_ALREADY_EXISTS) {
        LOG_E(TAG, "Failed to create %s", directory);
        return ERROR_RESOURCE;
    }

    PropertiesFile* file = properties_file_open(path);
    if (file == nullptr) {
        return ERROR_RESOURCE;
    }
    properties_file_set(file, KEY_REGION, radio_region_to_string(region));
    error = properties_file_close(file);
    if (error != ERROR_NONE) {
        LOG_E(TAG, "Failed to save %s", path);
        return error;
    }
    apply_to_devices(region);
    return ERROR_NONE;
}

void radio_settings_apply_region() {
    const RadioRegion region = radio_settings_load_region();
    if (region != RADIO_REGION_UNSET) {
        LOG_I(TAG, "Region %s", radio_region_to_string(region));
        apply_to_devices(region);
    }
}

}
