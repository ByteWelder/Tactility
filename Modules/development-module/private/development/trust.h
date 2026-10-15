// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <development/server.h>

#include <tactility/error.h>

/** @return true when @a fingerprint was stored with development_trust_add() in @a directory */
bool development_trust_contains(const char* directory, const char* fingerprint);

/**
 * Stores @a fingerprint in @a directory, so its connections are accepted without asking the user.
 * @param[in] name the client name, as reported by the client
 */
error_t development_trust_add(const char* directory, const char* fingerprint, const char* name);

/**
 * Removes @a fingerprint from @a directory.
 * @retval ERROR_NOT_FOUND it wasn't stored
 */
error_t development_trust_remove(const char* directory, const char* fingerprint);

/** Calls @a visitor for every stored client, in the order that they were added */
error_t development_trust_for_each(const char* directory, DevelopmentTrustedClientVisitor visitor, void* context);
