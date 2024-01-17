/** @file secure.h
 *
 * @brief Hardware-bound encryption methods.
 * @warning Enable secure boot and flash encryption to increase security.
 *
 * Offers AES 256 CBC encryption with built-in key.
 * The key is built from data including:
 *  - the internal factory MAC address
 *  - random data stored in NVS
 *
 * It's important to use flash encryption to avoid an attacker to get
 * access to your encrypted data. If flash encryption is disabled,
 * someone can fetch the key from the partitions.
 *
 * See:
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/secure-boot-v2.html
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/flash-encryption.html
 */
#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fills the IV with zeros and then copies up to 16 characters of the string into the IV.
 * @param input input text
 * @param iv output IV
 */
void tt_secure_get_iv_from_string(const char* input, uint8_t iv[16]);

/**
 * @brief Encrypt data.
 *
 * Important: Use flash encryption to increase security.
 * Important: input and output data must be aligned to 16 bytes.
 *
 * @param iv the AES IV
 * @param data_in input data
 * @param data_out output data
 * @param length data length, a multiple of 16
 * @return the result of esp_aes_crypt_cbc() (MBEDTLS_ERR_*)
 */
int tt_secure_encrypt(const uint8_t iv[16], uint8_t* in_data, uint8_t* out_data, size_t length);

/**
 * @brief Decrypt data.
 *
 * Important: Use flash encryption to increase security.
 * Important: input and output data must be aligned to 16 bytes.
 *
 * @param iv AES IV
 * @param data_in input data
 * @param data_out output data
 * @param length data length, a multiple of 16
 * @return the result of esp_aes_crypt_cbc() (MBEDTLS_ERR_*)
 */
int tt_secure_decrypt(const uint8_t iv[16], uint8_t* in_data, uint8_t* out_data, size_t length);

#ifdef __cplusplus
}
#endif