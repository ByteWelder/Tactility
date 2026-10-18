// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/module.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct Module location_module;

/**
 * @brief Sets the root certificate that verifies the IP lookup server.
 * @param[in] certificate_pem PEM text that must stay valid forever
 */
void location_configure_certificate(const char* certificate_pem);

#ifdef __cplusplus
}
#endif
