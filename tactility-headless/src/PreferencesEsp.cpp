#ifdef ESP_PLATFORM

#include "nvs_flash.h"
#include "preferences.h"
#include "tactility_core.h"

#define TAG "preferences"

bool Preferences::optBool(const char* key, bool* out) {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    } else {
        uint8_t out_number;
        bool success = nvs_get_u8(handle, key, &out_number) == ESP_OK;
        nvs_close(handle);
        if (success) {
            *out = (bool)out_number;
        }
        return success;
    }
}

bool Preferences::optInt32(const char* key, int32_t* out) {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    } else {
        bool success = nvs_get_i32(handle, key, out) == ESP_OK;
        nvs_close(handle);
        return success;
    }
}

bool Preferences::optString(const char* key, char* out, size_t* size) {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    } else {
        bool success = nvs_get_str(handle, key, out, size) == ESP_OK;
        nvs_close(handle);
        return success;
    }
}

bool Preferences::hasBool(const char* key) {
    bool temp;
    return optBool(key, &temp);
}

bool Preferences::hasInt32(const char* key) {
    int32_t temp;
    return optInt32(key, &temp);
}

bool Preferences::hasString(const char* key) {
    char temp[128];
    size_t temp_size = 128;
    return optString(key, temp, &temp_size);
}

void Preferences::putBool(const char* key, bool value) {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) == ESP_OK) {
        if (nvs_set_u8(handle, key, (uint8_t)value) != ESP_OK) {
            TT_LOG_E(TAG, "failed to write %s:%s", namespace_, key);
        }
        nvs_close(handle);
    } else {
        TT_LOG_E(TAG, "failed to open namespace %s for writing", namespace_);
    }
}

void Preferences::putInt32(const char* key, int32_t value) {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) == ESP_OK) {
        if (nvs_set_i32(handle, key, value) != ESP_OK) {
            TT_LOG_E(TAG, "failed to write %s:%s", namespace_, key);
        }
        nvs_close(handle);
    } else {
        TT_LOG_E(TAG, "failed to open namespace %s for writing", namespace_);
    }
}

void Preferences::putString(const char* key, const char* text) {
    nvs_handle_t handle;
    if (nvs_open(namespace_, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_str(handle, key, text);
        nvs_close(handle);
    } else {
        TT_LOG_E(TAG, "failed to open namespace %s for writing", namespace_);
    }
}

#endif