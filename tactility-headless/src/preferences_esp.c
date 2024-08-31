#ifdef ESP_PLATFORM

#include "preferences.h"
#include "nvs_flash.h"
#include "tactility_core.h"

#define TAG "preferences"

static bool opt_bool(const char* namespace, const char* key, bool* out) {
    nvs_handle_t handle;
    if (nvs_open(namespace, NVS_READWRITE, &handle) != ESP_OK) {
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

static bool opt_int32(const char* namespace, const char* key, int32_t* out) {
    nvs_handle_t handle;
    if (nvs_open(namespace, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    } else {
        bool success = nvs_get_i32(handle, key, out) == ESP_OK;
        nvs_close(handle);
        return success;
    }
}

static bool opt_string(const char* namespace, const char* key, char* out, size_t* size) {
    nvs_handle_t handle;
    if (nvs_open(namespace, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    } else {
        bool success = nvs_get_str(handle, key, out, size) == ESP_OK;
        nvs_close(handle);
        return success;
    }
}

static bool has_bool(const char* namespace, const char* key) {
    bool temp;
    return opt_bool(namespace, key, &temp);
}

static bool has_int32(const char* namespace, const char* key) {
    int32_t temp;
    return opt_int32(namespace, key, &temp);
}

static bool has_string(const char* namespace, const char* key) {
    char temp[128];
    size_t temp_size = 128;
    return opt_string(namespace, key, temp, &temp_size);
}

static void put_bool(const char* namespace, const char* key, bool value) {
    nvs_handle_t handle;
    if (nvs_open(namespace, NVS_READWRITE, &handle) == ESP_OK) {
        if (nvs_set_u8(handle, key, (uint8_t)value) != ESP_OK) {
            TT_LOG_E(TAG, "failed to write %s:%s", namespace, key);
        }
        nvs_close(handle);
    } else {
        TT_LOG_E(TAG, "failed to open namespace %s for writing", namespace);
    }
}

static void put_int32(const char* namespace, const char* key, int32_t value) {
    nvs_handle_t handle;
    if (nvs_open(namespace, NVS_READWRITE, &handle) == ESP_OK) {
        if (nvs_set_i32(handle, key, value) != ESP_OK) {
            TT_LOG_E(TAG, "failed to write %s:%s", namespace, key);
        }
        nvs_close(handle);
    } else {
        TT_LOG_E(TAG, "failed to open namespace %s for writing", namespace);
    }
}

static void put_string(const char* namespace, const char* key, const char* text) {
    nvs_handle_t handle;
    if (nvs_open(namespace, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_str(handle, key, text);
        nvs_close(handle);
    } else {
        TT_LOG_E(TAG, "failed to open namespace %s for writing", namespace);
    }
}

const Preferences preferences_esp = {
    .has_bool = &has_bool,
    .has_int32 = &has_int32,
    .has_string = &has_string,
    .opt_bool = &opt_bool,
    .opt_int32 = &opt_int32,
    .opt_string = &opt_string,
    .put_bool = &put_bool,
    .put_int32 = &put_int32,
    .put_string = &put_string
};

#endif