// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <development/server.h>

#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>

#include <stddef.h>
#include <stdint.h>

struct DevelopmentCredentials {
    mbedtls_x509_crt certificate;
    mbedtls_pk_context key;
    char fingerprint[DEVELOPMENT_SERVER_FINGERPRINT_LENGTH + 1];
};

/**
 * Loads the device certificate and key from @a directory, or generates and stores them there first.
 * @param[out] credentials must be released with development_credentials_free(), also on failure
 */
error_t development_credentials_load(const char* directory, DevelopmentCredentials* credentials);

void development_credentials_free(DevelopmentCredentials* credentials);

/** Writes the hex SHA-256 of @a der, which is DEVELOPMENT_SERVER_FINGERPRINT_LENGTH + 1 bytes with null terminator */
error_t development_fingerprint(const uint8_t* der, size_t length, char out_fingerprint[DEVELOPMENT_SERVER_FINGERPRINT_LENGTH + 1]);
