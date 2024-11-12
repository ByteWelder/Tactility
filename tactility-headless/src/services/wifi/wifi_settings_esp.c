#ifdef ESP_TARGET

#include "wifi_globals.h"
#include "wifi_settings.h"

#include "nvs_flash.h"
#include "log.h"
#include "hash.h"
#include "check.h"
#include "secure.h"

#define TAG "wifi_settings"
#define TT_NVS_NAMESPACE "wifi_settings" // limited by NVS_KEY_NAME_MAX_SIZE

// region Wi-Fi Credentials - static

static esp_err_t tt_wifi_credentials_nvs_open(nvs_handle_t* handle, nvs_open_mode_t mode) {
    return nvs_open(TT_NVS_NAMESPACE, NVS_READWRITE, handle);
}

static void tt_wifi_credentials_nvs_close(nvs_handle_t handle) {
    nvs_close(handle);
}

// endregion Wi-Fi Credentials - static

// region Wi-Fi Credentials - public

bool tt_wifi_settings_contains(const char* ssid) {
    nvs_handle_t handle;
    esp_err_t result = tt_wifi_credentials_nvs_open(&handle, NVS_READONLY);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle: %s", esp_err_to_name(result));
        return false;
    }

    bool key_exists = nvs_find_key(handle, ssid, NULL) == ESP_OK;
    tt_wifi_credentials_nvs_close(handle);

    return key_exists;
}

void tt_wifi_settings_init() {
    TT_LOG_I(TAG, "init");
    static_assert(strlen(TT_NVS_NAMESPACE) <= NVS_KEY_NAME_MAX_SIZE, "Namespace name too long");
}

bool tt_wifi_settings_load(const char* ssid, WifiApSettings* settings) {
    nvs_handle_t handle;
    esp_err_t result = tt_wifi_credentials_nvs_open(&handle, NVS_READONLY);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle: %s", esp_err_to_name(result));
        return false;
    }

    WifiApSettings encrypted_settings;
    size_t length = sizeof(WifiApSettings);
    result = nvs_get_blob(handle, ssid, &encrypted_settings, &length);

    uint8_t iv[16];
    tt_secure_get_iv_from_string(ssid, iv);
    int decrypt_result = tt_secure_decrypt(
        iv,
        (uint8_t*)encrypted_settings.secret,
        (uint8_t*)settings->secret,
        TT_WIFI_CREDENTIALS_PASSWORD_LIMIT
    );
    // Manually ensure null termination, because encryption must be a multiple of 16 bytes
    encrypted_settings.secret[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT] = 0;

    if (decrypt_result != 0) {
        result = ESP_FAIL;
        TT_LOG_E(TAG, "Failed to decrypt credentials for \"%s\": %d", ssid, decrypt_result);
    }

    if (result != ESP_OK && result != ESP_ERR_NVS_NOT_FOUND) {
        TT_LOG_E(TAG, "Failed to get credentials for \"%s\": %s", ssid, esp_err_to_name(result));
    }

    tt_wifi_credentials_nvs_close(handle);

    settings->auto_connect = encrypted_settings.auto_connect;
    strcpy((char*)settings->ssid, encrypted_settings.ssid);

    return result == ESP_OK;
}

bool tt_wifi_settings_save(const WifiApSettings* settings) {
    nvs_handle_t handle;
    esp_err_t result = tt_wifi_credentials_nvs_open(&handle, NVS_READWRITE);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle: %s", esp_err_to_name(result));
        return false;
    }

    WifiApSettings encrypted_settings = {
        .auto_connect = settings->auto_connect,
    };
    strcpy((char*)encrypted_settings.ssid, settings->ssid);
    // We only decrypt multiples of 16, so we have to ensure the last byte is set to 0
    encrypted_settings.secret[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT] = 0;

    uint8_t iv[16];
    tt_secure_get_iv_from_data(settings->ssid, strlen(settings->ssid), iv);
    int encrypt_result = tt_secure_encrypt(
        iv,
        (uint8_t*)settings->secret,
        (uint8_t*)encrypted_settings.secret,
        TT_WIFI_CREDENTIALS_PASSWORD_LIMIT
    );

    if (encrypt_result != 0) {
        result = ESP_FAIL;
        TT_LOG_E(TAG, "Failed to encrypt credentials \"%s\": %d", settings->ssid, encrypt_result);
    }

    if (result == ESP_OK) {
        result = nvs_set_blob(handle, settings->ssid, &encrypted_settings, sizeof(WifiApSettings));
        if (result != ESP_OK) {
            TT_LOG_E(TAG, "Failed to get credentials for \"%s\": %s", settings->ssid, esp_err_to_name(result));
        }
    }

    tt_wifi_credentials_nvs_close(handle);
    return result == ESP_OK;
}

bool tt_wifi_settings_remove(const char* ssid) {
    nvs_handle_t handle;
    esp_err_t result = tt_wifi_credentials_nvs_open(&handle, NVS_READWRITE);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle to store \"%s\": %s", ssid, esp_err_to_name(result));
        return false;
    }

    result = nvs_erase_key(handle, ssid);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to erase credentials for \"%s\": %s", ssid, esp_err_to_name(result));
    }

    tt_wifi_credentials_nvs_close(handle);
    return result == ESP_OK;
}

// end region Wi-Fi Credentials - public

#endif // ESP_TARGET