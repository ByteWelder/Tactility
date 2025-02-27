#ifdef ESP_PLATFORM

#include "nvs_flash.h"
#include "Tactility/Preferences.h"

#include <Tactility/TactilityCore.h>

#define TAG "preferences"

namespace tt {

bool Preferences::optBool(const std::string& key, bool& out) const {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open namespace %s", namespace_);
        return false;
    } else {
        uint8_t out_number;
        bool success = nvs_get_u8(handle, key.c_str(), &out_number) == ESP_OK;
        nvs_close(handle);
        if (success) {
            out = (bool)out_number;
        }
        return success;
    }
}

bool Preferences::optInt32(const std::string& key, int32_t& out) const {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open namespace %s", namespace_);
        return false;
    } else {
        bool success = nvs_get_i32(handle, key.c_str(), &out) == ESP_OK;
        nvs_close(handle);
        return success;
    }
}

bool Preferences::optString(const std::string& key, std::string& out) const {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open namespace %s", namespace_);
        return false;
    } else {
        size_t out_size = 256;
        char* out_data = static_cast<char*>(malloc(out_size));
        bool success = nvs_get_str(handle, key.c_str(), out_data, &out_size) == ESP_OK;
        nvs_close(handle);
        out = out_data;
        free(out_data);
        return success;
    }
}

bool Preferences::hasBool(const std::string& key) const {
    bool temp;
    return optBool(key, temp);
}

bool Preferences::hasInt32(const std::string& key) const {
    int32_t temp;
    return optInt32(key, temp);
}

bool Preferences::hasString(const std::string& key) const {
    std::string temp;
    return optString(key, temp);
}

void Preferences::putBool(const std::string& key, bool value) {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) == ESP_OK) {
        if (nvs_set_u8(handle, key.c_str(), (uint8_t)value) != ESP_OK) {
            TT_LOG_E(TAG, "Failed to write %s:%s", namespace_, key.c_str());
        }
        nvs_close(handle);
    } else {
        TT_LOG_E(TAG, "Failed to open namespace %s", namespace_);
    }
}

void Preferences::putInt32(const std::string& key, int32_t value) {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) == ESP_OK) {
        if (nvs_set_i32(handle, key.c_str(), value) != ESP_OK) {
            TT_LOG_E(TAG, "Failed to write %s:%s", namespace_, key.c_str());
        }
        nvs_close(handle);
    } else {
        TT_LOG_E(TAG, "Failed to open namespace %s", namespace_);
    }
}

void Preferences::putString(const std::string& key, const std::string& text) {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_str(handle, key.c_str(), text.c_str());
        nvs_close(handle);
    } else {
        TT_LOG_E(TAG, "Failed to open namespace %s", namespace_);
    }
}

} // namespace

#endif