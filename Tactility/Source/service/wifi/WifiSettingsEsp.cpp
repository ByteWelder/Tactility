#ifdef ESP_PLATFORM

#include "Tactility/service/wifi/WifiGlobals.h"
#include "Tactility/service/wifi/WifiSettings.h"

#include <Tactility/Log.h>
#include <Tactility/crypt/Hash.h>
#include <Tactility/crypt/Crypt.h>

#include <nvs_flash.h>
#include <cstring>

#define TAG "wifi_settings"
#define TT_NVS_NAMESPACE "wifi_settings" // limited by NVS_KEY_NAME_MAX_SIZE

// region Wi-Fi Credentials - static

static esp_err_t credentials_nvs_open(nvs_handle_t* handle, nvs_open_mode_t mode) {
    return nvs_open(TT_NVS_NAMESPACE, NVS_READWRITE, handle);
}

static void credentials_nvs_close(nvs_handle_t handle) {
    nvs_close(handle);
}

// endregion Wi-Fi Credentials - static

// region Wi-Fi Credentials - public
namespace tt::service::wifi::settings {

bool contains(const char* ssid) {
    nvs_handle_t handle;
    esp_err_t result = credentials_nvs_open(&handle, NVS_READONLY);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle: %s", esp_err_to_name(result));
        return false;
    }

    bool key_exists = nvs_find_key(handle, ssid, NULL) == ESP_OK;
    credentials_nvs_close(handle);

    return key_exists;
}

bool load(const char* ssid, WifiApSettings* settings) {
    nvs_handle_t handle;
    esp_err_t result = credentials_nvs_open(&handle, NVS_READONLY);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle: %s", esp_err_to_name(result));
        return false;
    }

    WifiApSettings encrypted_settings;
    size_t length = sizeof(WifiApSettings);
    result = nvs_get_blob(handle, ssid, &encrypted_settings, &length);

    uint8_t iv[16];
    crypt::getIv(ssid, strlen(ssid), iv);
    int decrypt_result = crypt::decrypt(
        iv,
        (uint8_t*)encrypted_settings.password,
        (uint8_t*)settings->password,
        TT_WIFI_CREDENTIALS_PASSWORD_LIMIT
    );
    // Manually ensure null termination, because encryption must be a multiple of 16 bytes
    encrypted_settings.password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT] = 0;

    if (decrypt_result != 0) {
        result = ESP_FAIL;
        TT_LOG_E(TAG, "Failed to decrypt credentials for \"%s\": %d", ssid, decrypt_result);
    }

    if (result != ESP_OK && result != ESP_ERR_NVS_NOT_FOUND) {
        TT_LOG_E(TAG, "Failed to get credentials for \"%s\": %s", ssid, esp_err_to_name(result));
    }

    credentials_nvs_close(handle);

    settings->auto_connect = encrypted_settings.auto_connect;
    strcpy((char*)settings->ssid, encrypted_settings.ssid);

    return result == ESP_OK;
}

bool save(const WifiApSettings* settings) {
    nvs_handle_t handle;
    esp_err_t result = credentials_nvs_open(&handle, NVS_READWRITE);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle: %s", esp_err_to_name(result));
        return false;
    }

    WifiApSettings encrypted_settings = {
        .ssid = { 0 },
        .password = { 0 },
        .auto_connect = settings->auto_connect,
    };
    strcpy((char*)encrypted_settings.ssid, settings->ssid);
    // We only decrypt multiples of 16, so we have to ensure the last byte is set to 0
    encrypted_settings.password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT] = 0;

    uint8_t iv[16];
    crypt::getIv(settings->ssid, strlen(settings->ssid), iv);
    int encrypt_result = crypt::encrypt(
        iv,
        (uint8_t*)settings->password,
        (uint8_t*)encrypted_settings.password,
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

    credentials_nvs_close(handle);
    return result == ESP_OK;
}

bool remove(const char* ssid) {
    nvs_handle_t handle;
    esp_err_t result = credentials_nvs_open(&handle, NVS_READWRITE);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle to store \"%s\": %s", ssid, esp_err_to_name(result));
        return false;
    }

    result = nvs_erase_key(handle, ssid);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to erase credentials for \"%s\": %s", ssid, esp_err_to_name(result));
    }

    credentials_nvs_close(handle);
    return result == ESP_OK;
}

// end region Wi-Fi Credentials - public

} // nemespace

#endif // ESP_PLATFORM