#ifdef ESP_TARGET

#include "wifi_credentials.h"

#include "nvs_flash.h"
#include "log.h"
#include "hash.h"
#include "check.h"
#include "mutex.h"
#include "secure.h"

#define TAG "wifi_credentials"
#define TT_NVS_NAMESPACE "tt_wifi_cred" // limited by NVS_KEY_NAME_MAX_SIZE
#define TT_NVS_PARTITION "nvs"

static void hash_reset_all();

// region Hash

static Mutex* hash_mutex = NULL;
static int8_t hash_index = -1;
static uint32_t hashes[TT_WIFI_CREDENTIALS_LIMIT] = { 0 };

static void hash_init() {
    tt_assert(hash_mutex == NULL);
    hash_mutex = tt_mutex_alloc(MutexTypeNormal);
    hash_reset_all();
}

static void tt_hash_mutex_lock() {
    tt_assert(tt_mutex_acquire(hash_mutex, 100) == TtStatusOk);
}

static void tt_hash_mutex_unlock() {
    tt_assert(tt_mutex_release(hash_mutex) == TtStatusOk);
}

static int hash_find_value(uint32_t hash) {
    tt_hash_mutex_lock();
    for (int i = 0; i < hash_index; ++i) {
        if (hashes[i] == hash) {
            tt_hash_mutex_unlock();
            return i;
        }
    }
    tt_hash_mutex_unlock();
    return -1;
}

static bool hash_contains_value(uint32_t value) {
    return hash_find_value(value) != -1;
}

static void hash_add(const char* ssid) {
    uint32_t hash = tt_hash_string_djb2(ssid);
    if (!hash_contains_value(hash)) {
        tt_hash_mutex_lock();
        tt_check((hash_index + 1) < TT_WIFI_CREDENTIALS_LIMIT, "exceeding wifi credentials list size");
        hash_index++;
        hashes[hash_index] = hash;
        tt_hash_mutex_unlock();
    }
}

static void hash_reset_all() {
    hash_index = -1;
}

// endregion Hash

// region Wi-Fi Credentials - static

static esp_err_t tt_wifi_credentials_nvs_open(nvs_handle_t* handle, nvs_open_mode_t mode) {
    return nvs_open(TT_NVS_NAMESPACE, NVS_READWRITE, handle);
}

static void tt_wifi_credentials_nvs_close(nvs_handle_t handle) {
    nvs_close(handle);
}

static bool tt_wifi_credentials_contains_in_flash(const char* ssid) {
    nvs_handle_t handle;
    esp_err_t result = tt_wifi_credentials_nvs_open(&handle, NVS_READONLY);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle: %s", esp_err_to_name(result));
        return false;
    }

    hash_reset_all();

    nvs_iterator_t iterator;
    result = nvs_entry_find(TT_NVS_PARTITION, TT_NVS_NAMESPACE, NVS_TYPE_BLOB, &iterator);
    bool contains_ssid = false;
    while (result == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(iterator, &info); // Can omit error check if parameters are guaranteed to be non-NULL
        if (strcmp(info.key, ssid) == 0) {
            contains_ssid = true;
            break;
        }
        result = nvs_entry_next(&iterator);
    }

    nvs_release_iterator(iterator);
    tt_wifi_credentials_nvs_close(handle);

    return contains_ssid;
}

// endregion Wi-Fi Credentials - static

// region Wi-Fi Credentials - public

bool tt_wifi_credentials_contains(const char* ssid) {
    uint32_t hash = tt_hash_string_djb2(ssid);
    if (hash_contains_value(hash)) {
        return tt_wifi_credentials_contains_in_flash(ssid);
    } else {
        return false;
    }
}

void tt_wifi_credentials_init() {
    TT_LOG_I(TAG, "init started");
    hash_init();

    nvs_handle_t handle;
    esp_err_t result = tt_wifi_credentials_nvs_open(&handle, NVS_READWRITE);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle for init: %s", esp_err_to_name(result));
        return;
    }

    nvs_iterator_t iterator;
    result = nvs_entry_find(TT_NVS_PARTITION, TT_NVS_NAMESPACE, NVS_TYPE_BLOB, &iterator);
    while (result == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(iterator, &info); // Can omit error check if parameters are guaranteed to be non-NULL
        hash_add(info.key);
        result = nvs_entry_next(&iterator);
    }
    nvs_release_iterator(iterator);

    tt_wifi_credentials_nvs_close(handle);
    TT_LOG_I(TAG, "init finished");
}

bool tt_wifi_credentials_get(const char* ssid, char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT]) {
    nvs_handle_t handle;
    esp_err_t result = tt_wifi_credentials_nvs_open(&handle, NVS_READONLY);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle: %s", esp_err_to_name(result));
        return false;
    }

    char password_encrypted[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT];
    size_t length = TT_WIFI_CREDENTIALS_PASSWORD_LIMIT;
    result = nvs_get_blob(handle, ssid, password_encrypted, &length);

    uint8_t iv[16];
    tt_secure_get_iv_from_string(ssid, iv);
    int decrypt_result = tt_secure_decrypt(
        iv,
        (uint8_t*)password_encrypted,
        (uint8_t*)password,
        TT_WIFI_CREDENTIALS_PASSWORD_LIMIT
    );

    if (decrypt_result != 0) {
        result = ESP_FAIL;
        TT_LOG_E(TAG, "Failed to decrypt credentials for \"%s\": %d", ssid, decrypt_result);
    }

    if (result != ESP_OK && result != ESP_ERR_NVS_NOT_FOUND) {
        TT_LOG_E(TAG, "Failed to get credentials for \"%s\": %s", ssid, esp_err_to_name(result));
    }

    tt_wifi_credentials_nvs_close(handle);
    return result == ESP_OK;
}

bool tt_wifi_credentials_set(const char* ssid, char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT]) {
    nvs_handle_t handle;
    esp_err_t result = tt_wifi_credentials_nvs_open(&handle, NVS_READWRITE);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to open NVS handle: %s", esp_err_to_name(result));
        return false;
    }

    char password_encrypted[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT];
    uint8_t iv[16];
    tt_secure_get_iv_from_string(ssid, iv);
    int encrypt_result = tt_secure_encrypt(
        iv,
        (uint8_t*)password,
        (uint8_t*)password_encrypted,
        TT_WIFI_CREDENTIALS_PASSWORD_LIMIT
    );

    if (encrypt_result != 0) {
        result = ESP_FAIL;
        TT_LOG_E(TAG, "Failed to encrypt credentials for \"%s\": %d", ssid, encrypt_result);
    }

    if (result == ESP_OK) {
        result = nvs_set_blob(handle, ssid, password_encrypted, TT_WIFI_CREDENTIALS_PASSWORD_LIMIT);
        if (result != ESP_OK) {
            TT_LOG_E(TAG, "Failed to get credentials for \"%s\": %s", ssid, esp_err_to_name(result));
        }
    }

    tt_wifi_credentials_nvs_close(handle);
    return result == ESP_OK;
}

bool tt_wifi_credentials_remove(const char* ssid) {
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