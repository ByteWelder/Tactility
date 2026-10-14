// SPDX-License-Identifier: Apache-2.0
#include <development/credentials.h>

#include <crypt/crypt.h>

#include <tactility/filesystem/fs.h>
#include <tactility/log.h>
#include <tactility/memory.h>
#include <tactility/paths.h>

#include <mbedtls/platform_util.h>
#include <mbedtls/x509_crt.h>
#include <psa/crypto.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr auto* TAG = "DevelopmentCredentials";

constexpr auto* KEY_FILE = "server.key";
constexpr auto* CERTIFICATE_FILE = "server.crt";
constexpr auto* SUBJECT_NAME = "CN=Tactility,O=Tactility";

constexpr size_t IV_SIZE = 16;
constexpr size_t BLOCK_SIZE = 16;
constexpr size_t KEY_DER_MAX = 256;
constexpr size_t KEY_FILE_MAX = IV_SIZE + KEY_DER_MAX + BLOCK_SIZE;
constexpr size_t CERTIFICATE_DER_MAX = 1024;

constexpr MemoryPolicy MEMORY_POLICY = { .required = 0, .desired = MEMORY_CAPABILITY_EXTERNAL, .alignment = 0 };

bool getPath(const char* directory, const char* file, char* outPath, size_t outSize) {
    const int written = snprintf(outPath, outSize, "%s/%s", directory, file);
    return written > 0 && static_cast<size_t>(written) < outSize;
}

bool writeFile(const char* directory, const char* file, const uint8_t* data, size_t length) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (!getPath(directory, file, path, sizeof(path))) {
        return false;
    }
    FILE* handle = fopen(path, "wb");
    if (handle == nullptr) {
        LOG_E(TAG, "Failed to open %s", path);
        return false;
    }
    const bool written = fwrite(data, 1, length, handle) == length;
    return fclose(handle) == 0 && written;
}

bool readFile(const char* directory, const char* file, uint8_t* data, size_t* inOutLength) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    return getPath(directory, file, path, sizeof(path)) && file_read_binary(path, data, inOutLength) == ERROR_NONE;
}

// File layout: IV, then the encrypted [u16 big-endian DER length][DER][zero padding]
bool writeKey(const char* directory, const uint8_t* der, size_t derLength) {
    uint8_t plain[KEY_DER_MAX + BLOCK_SIZE] = {};
    plain[0] = static_cast<uint8_t>(derLength >> 8);
    plain[1] = static_cast<uint8_t>(derLength & 0xFF);
    memcpy(plain + 2, der, derLength);
    const size_t encryptedLength = ((derLength + 2 + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;

    uint8_t file[KEY_FILE_MAX];
    crypt_generate_iv(file);
    const bool encrypted = crypt_encrypt(file, plain, file + IV_SIZE, encryptedLength) == 0;
    mbedtls_platform_zeroize(plain, sizeof(plain));
    return encrypted && writeFile(directory, KEY_FILE, file, IV_SIZE + encryptedLength);
}

bool readKey(const char* directory, uint8_t* outDer, size_t* outLength) {
    uint8_t file[KEY_FILE_MAX];
    size_t fileLength = sizeof(file);
    if (!readFile(directory, KEY_FILE, file, &fileLength) || fileLength <= IV_SIZE || (fileLength - IV_SIZE) % BLOCK_SIZE != 0) {
        return false;
    }
    uint8_t plain[KEY_DER_MAX + BLOCK_SIZE];
    const size_t encryptedLength = fileLength - IV_SIZE;
    if (crypt_decrypt(file, file + IV_SIZE, plain, encryptedLength) != 0) {
        return false;
    }
    const size_t derLength = (static_cast<size_t>(plain[0]) << 8) | plain[1];
    const bool valid = derLength > 0 && derLength + 2 <= encryptedLength;
    if (valid) {
        memcpy(outDer, plain + 2, derLength);
        *outLength = derLength;
    }
    mbedtls_platform_zeroize(plain, sizeof(plain));
    return valid;
}

bool generateKey(mbedtls_pk_context* pk) {
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_EXPORT);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));

    mbedtls_svc_key_id_t keyId = MBEDTLS_SVC_KEY_ID_INIT;
    if (psa_generate_key(&attributes, &keyId) != PSA_SUCCESS) {
        return false;
    }
    const bool copied = mbedtls_pk_copy_from_psa(keyId, pk) == 0;
    psa_destroy_key(keyId);
    return copied;
}

bool writeCertificate(mbedtls_pk_context* pk, uint8_t* buffer, size_t bufferSize, const uint8_t** outDer, size_t* outLength) {
    uint8_t serial[16];
    if (psa_generate_random(serial, sizeof(serial)) != PSA_SUCCESS) {
        return false;
    }
    // Keeps the serial number positive
    serial[0] &= 0x7F;

    mbedtls_x509write_cert writer;
    mbedtls_x509write_crt_init(&writer);
    mbedtls_x509write_crt_set_version(&writer, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&writer, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_key(&writer, pk);
    mbedtls_x509write_crt_set_issuer_key(&writer, pk);
    int result = mbedtls_x509write_crt_set_serial_raw(&writer, serial, sizeof(serial));
    if (result == 0) result = mbedtls_x509write_crt_set_subject_name(&writer, SUBJECT_NAME);
    if (result == 0) result = mbedtls_x509write_crt_set_issuer_name(&writer, SUBJECT_NAME);
    // Devices often have no real-time clock, so the validity covers any plausible date
    if (result == 0) result = mbedtls_x509write_crt_set_validity(&writer, "20000101000000", "20991231235959");
    if (result == 0) result = mbedtls_x509write_crt_der(&writer, buffer, bufferSize);
    mbedtls_x509write_crt_free(&writer);
    if (result <= 0) {
        LOG_E(TAG, "Failed to write certificate: -0x%04x", static_cast<unsigned>(-result));
        return false;
    }
    // The DER is written at the end of the buffer
    *outLength = static_cast<size_t>(result);
    *outDer = buffer + bufferSize - *outLength;
    return true;
}

bool generate(const char* directory) {
    LOG_I(TAG, "Generating device certificate");
    auto* buffer = static_cast<uint8_t*>(memory_alloc_with_policy(KEY_DER_MAX + CERTIFICATE_DER_MAX, &MEMORY_POLICY));
    if (buffer == nullptr) {
        return false;
    }
    uint8_t* keyBuffer = buffer;
    uint8_t* certificateBuffer = buffer + KEY_DER_MAX;

    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    bool success = generateKey(&pk);

    const uint8_t* certificateDer = nullptr;
    size_t certificateLength = 0;
    success = success && writeCertificate(&pk, certificateBuffer, CERTIFICATE_DER_MAX, &certificateDer, &certificateLength);

    int keyLength = success ? mbedtls_pk_write_key_der(&pk, keyBuffer, KEY_DER_MAX) : 0;
    success = keyLength > 0 &&
        writeKey(directory, keyBuffer + KEY_DER_MAX - keyLength, static_cast<size_t>(keyLength)) &&
        writeFile(directory, CERTIFICATE_FILE, certificateDer, certificateLength);

    mbedtls_pk_free(&pk);
    mbedtls_platform_zeroize(keyBuffer, KEY_DER_MAX);
    memory_free(buffer);
    return success;
}

bool parse(const char* directory, DevelopmentCredentials* credentials) {
    auto* buffer = static_cast<uint8_t*>(memory_alloc_with_policy(KEY_DER_MAX + CERTIFICATE_DER_MAX, &MEMORY_POLICY));
    if (buffer == nullptr) {
        return false;
    }
    uint8_t* keyDer = buffer;
    uint8_t* certificateDer = buffer + KEY_DER_MAX;
    size_t keyLength = 0;
    size_t certificateLength = CERTIFICATE_DER_MAX;
    const bool success = readKey(directory, keyDer, &keyLength) &&
        readFile(directory, CERTIFICATE_FILE, certificateDer, &certificateLength) &&
        mbedtls_pk_parse_key(&credentials->key, keyDer, keyLength, nullptr, 0) == 0 &&
        mbedtls_x509_crt_parse_der(&credentials->certificate, certificateDer, certificateLength) == 0 &&
        development_fingerprint(certificateDer, certificateLength, credentials->fingerprint) == ERROR_NONE;
    mbedtls_platform_zeroize(keyDer, KEY_DER_MAX);
    memory_free(buffer);
    return success;
}

void reset(DevelopmentCredentials* credentials) {
    development_credentials_free(credentials);
    mbedtls_x509_crt_init(&credentials->certificate);
    mbedtls_pk_init(&credentials->key);
    credentials->fingerprint[0] = '\0';
}

} // namespace

error_t development_credentials_load(const char* directory, DevelopmentCredentials* credentials) {
    mbedtls_x509_crt_init(&credentials->certificate);
    mbedtls_pk_init(&credentials->key);
    credentials->fingerprint[0] = '\0';

    if (psa_crypto_init() != PSA_SUCCESS) {
        return ERROR_RESOURCE;
    }

    if (parse(directory, credentials)) {
        return ERROR_NONE;
    }
    reset(credentials);

    const error_t directoryError = directory_make(directory, true);
    if (directoryError != ERROR_NONE && directoryError != ERROR_ALREADY_EXISTS) {
        LOG_E(TAG, "Failed to create %s", directory);
        return ERROR_RESOURCE;
    }
    if (!generate(directory) || !parse(directory, credentials)) {
        LOG_E(TAG, "Failed to create device certificate");
        return ERROR_RESOURCE;
    }
    return ERROR_NONE;
}

void development_credentials_free(DevelopmentCredentials* credentials) {
    mbedtls_x509_crt_free(&credentials->certificate);
    mbedtls_pk_free(&credentials->key);
}

error_t development_fingerprint(const uint8_t* der, size_t length, char out_fingerprint[DEVELOPMENT_SERVER_FINGERPRINT_LENGTH + 1]) {
    uint8_t hash[32];
    size_t hashLength = 0;
    if (psa_hash_compute(PSA_ALG_SHA_256, der, length, hash, sizeof(hash), &hashLength) != PSA_SUCCESS || hashLength != sizeof(hash)) {
        return ERROR_RESOURCE;
    }
    for (size_t i = 0; i < sizeof(hash); i++) {
        snprintf(&out_fingerprint[i * 2], 3, "%02x", hash[i]);
    }
    return ERROR_NONE;
}
