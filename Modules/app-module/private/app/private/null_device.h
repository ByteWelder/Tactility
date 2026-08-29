// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <app/file.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @return the shared null-device AppFile: write() discards data and reports success, read()
 * reports EOF, close() is a no-op, and it is always reported readable/writable.
 */
const struct AppFile* app_null_file(void);

#ifdef __cplusplus
}
#endif
