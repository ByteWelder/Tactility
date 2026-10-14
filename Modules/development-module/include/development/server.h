// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEVELOPMENT_SERVER_SERVICE_ID "development-server"

#define DEVELOPMENT_SERVER_DEFAULT_PORT 6667

/** Hex SHA-256 of a DER certificate, excluding null terminator */
#define DEVELOPMENT_SERVER_FINGERPRINT_LENGTH 64

/** Maximum length of the name that a client reports, excluding null terminator */
#define DEVELOPMENT_SERVER_CLIENT_NAME_LENGTH 63

/**
 * The app that asks the user to accept or reject an incoming connection.
 * It is started as a child of the connection's session, with argv:
 *   [0] the peer IP address
 *   [1] the client name, as reported by the client
 *   [2] the client certificate fingerprint (see DEVELOPMENT_SERVER_FINGERPRINT_LENGTH)
 *   [3] the requested mode (e.g. "shell")
 * Its result must be a DevelopmentPromptResult. Any other result rejects the connection.
 */
#define DEVELOPMENT_SERVER_PROMPT_APP_ID "DevelopmentPrompt"

enum DevelopmentPromptResult {
    DEVELOPMENT_PROMPT_REJECT = 0,
    DEVELOPMENT_PROMPT_ACCEPT_ONCE = 1,
    DEVELOPMENT_PROMPT_ACCEPT_ALWAYS = 2,
};

/**
 * Starts or stops listening for connections, and persists the choice for the next boot.
 * @retval ERROR_INVALID_STATE the service is not running
 * @retval ERROR_RESOURCE the listener could not be started
 * @retval ERROR_NONE on success
 */
error_t development_server_set_enabled(bool enabled);

/** @return true when the server is listening for connections */
bool development_server_is_enabled(void);

/**
 * Writes the fingerprint of the device certificate, which clients use to identify the device.
 * @param[out] out_fingerprint at least DEVELOPMENT_SERVER_FINGERPRINT_LENGTH + 1 bytes
 * @retval ERROR_INVALID_STATE the service is not running
 * @retval ERROR_BUFFER_OVERFLOW @a out_size is too small
 * @retval ERROR_NONE on success
 */
error_t development_server_get_fingerprint(char* out_fingerprint, size_t out_size);

/** @return the TCP port that the server listens on */
unsigned short development_server_get_port(void);

/**
 * @param[in] fingerprint see DEVELOPMENT_SERVER_FINGERPRINT_LENGTH
 * @param[in] name the client name, as reported by the client when it was trusted
 * @return true to continue with the next client
 */
typedef bool (*DevelopmentTrustedClientVisitor)(const char* fingerprint, const char* name, void* context);

/**
 * Calls @a visitor for every client that the user chose to always accept, in the order that they were added.
 * @warning @a visitor must not call other trusted client functions
 * @retval ERROR_INVALID_STATE the service is not running
 * @retval ERROR_NONE on success
 */
error_t development_server_for_each_trusted_client(DevelopmentTrustedClientVisitor visitor, void* context);

/**
 * Stops trusting a client, so its next connection asks the user again. Active sessions continue.
 * @retval ERROR_INVALID_STATE the service is not running
 * @retval ERROR_NOT_FOUND the client isn't trusted
 * @retval ERROR_NONE on success
 */
error_t development_server_remove_trusted_client(const char* fingerprint);

#ifdef __cplusplus
}
#endif
