#include "secure.h"

#include "aes/esp_aes.h"
#include "check.h"
#include "esp_mac.h"
#include "esp_cpu.h"
#include "log.h"
#include "nvs_flash.h"
#include <string.h>

#define TAG "secure"
#define TT_NVS_NAMESPACE "tt_secure"

/**
 * Get a key based on hardware parameters.
 * @param[out] key the output key
 */
static void get_hardware_key(uint8_t key[32]) {
    uint8_t mac[8];
    // MAC can be 6 or 8 bytes
    size_t mac_length = esp_mac_addr_len_get(ESP_MAC_EFUSE_FACTORY);
    TT_LOG_I(TAG, "Using MAC with length %u", mac_length);
    tt_check(mac_length <= 8);
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_EFUSE_FACTORY));

    // Fill buffer with repeating MAC
    for (size_t i = 0; i < 32; ++i) {
        key[i] = mac[i % mac_length];
    }
}

/**
 * The key is built up as follows:
 * - Fetch 32 bytes from NVS storage and store as key data
 * - Fetch 6-8 MAC bytes and overwrite the first 6-8 bytes of the key with this info
 *
 * When flash encryption is disabled:
 *   Without the MAC data, an attack would look like this:
 *   - Retrieve all the partitions from the ESP32
 *   - Read the key from NVS flash
 *   - Use the key to decrypt
 *   With the MAC data added, an attacker would have to do much more:
 *   - Retrieve all the partitions from the ESP32 (copy app)
 *   - Upload custom app to retrieve internal MAC
 *   - Read the key from NVS flash
 *   - Re-flash original app and combine it with the MAC
 *   - Use the key to decrypt
 *   - Re-flash the device with original firmware.
 *
 * Adding the MAC doesn't add a lot of extra security, but I think it's worth it.
 *
 * @param[out] key the output key
 */
static void get_nvs_key(uint8_t key[32]) {
    nvs_handle_t handle;
    esp_err_t result = nvs_open(TT_NVS_NAMESPACE, NVS_READWRITE, &handle);

    if (result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to get key from NVS (%s)", esp_err_to_name(result));
        tt_crash();
    }

    size_t length = 32;
    if (nvs_get_blob(handle, "key", key, &length) == ESP_OK) {
        TT_LOG_I(TAG, "Fetched key from NVS (%d bytes)", length);
        tt_check(length == 32);
    } else {
        esp_cpu_cycle_count_t cycle_count = esp_cpu_get_cycle_count();
        unsigned seed = (unsigned)cycle_count;
        for (int i = 0; i < 32; ++i) {
            key[i] = (uint8_t)rand_r(&seed);
        }
        ESP_ERROR_CHECK(nvs_set_blob(handle, "key", key, 32));
        TT_LOG_I(TAG, "Stored new key in NVS");
    }

    nvs_close(handle);
}

/**
 * Performs XOR on 2 memory regions and stores it in a third
 * @param[in] in_left input buffer for XOR
 * @param[in] in_right second input buffer for XOR
 * @param[out] out output buffer for result of XOR
 * @param[in] length data length (all buffers must be at least this size)
 */
static void xor_key(const uint8_t* in_left, const uint8_t* in_right, uint8_t* out, size_t length) {
    for (int i = 0; i < length; ++i) {
        out[i] = in_left[i] ^ in_right[i];
    }
}

/**
 * Combines a stored key and a hardware key into a single reliable key value.
 * @param[out] key the key output
 */
static void get_key(uint8_t key[32]) {
#if !defined(CONFIG_SECURE_BOOT) || !defined(CONFIG_SECURE_FLASH_ENC_ENABLED)
    TT_LOG_W(TAG, "Using tt_secure_* code with secure boot and/or flash encryption disabled.");
    TT_LOG_W(TAG, "An attacker with physical access to your ESP32 can decrypt your secure data.");
#endif

    uint8_t hardware_key[32];
    uint8_t nvs_key[32];

    get_hardware_key(hardware_key);
    get_nvs_key(nvs_key);
    xor_key(hardware_key, nvs_key, key, 32);
}

void tt_secure_get_iv_from_string(const char* input, uint8_t iv[16]) {
    memset((void*)iv, 0, 16);
    char c = *input++;
    int index = 0;
    printf("IV: ");
    while (c) {
        printf(" %0X:%02d", c, index);
        iv[index] = c;
        index++;
        c = *input++;
    }
    printf("\n");
}

int tt_secure_encrypt(const uint8_t iv[16], uint8_t* in_data, uint8_t* out_data, size_t length) {
    tt_check(length % 16 == 0, "Length is not a multiple of 16 bytes (for AES 256");
    uint8_t key[32];
    get_key(key);

    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, sizeof(iv_copy));

    esp_aes_context ctx;
    esp_aes_init(&ctx);
    esp_aes_setkey(&ctx, key, 256);
    int result = esp_aes_crypt_cbc(&ctx, ESP_AES_ENCRYPT, length, iv_copy, in_data, out_data);
    esp_aes_free(&ctx);
    return result;
}

int tt_secure_decrypt(const uint8_t iv[16], uint8_t* in_data, uint8_t* out_data, size_t length) {
    tt_check(length % 16 == 0, "Length is not a multiple of 16 bytes (for AES 256");
    uint8_t key[32];
    get_key(key);

    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, sizeof(iv_copy));

    esp_aes_context ctx;
    esp_aes_init(&ctx);
    esp_aes_setkey(&ctx, key, 256);
    int result = esp_aes_crypt_cbc(&ctx, ESP_AES_DECRYPT, length, iv_copy, in_data, out_data);
    esp_aes_free(&ctx);
    return result;
}
