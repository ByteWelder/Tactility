// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <app/file.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @return the AppFileOps used for stream-backed fd table entries (see app_stream_subscribe()). */
const struct AppFileOps* app_stream_ops(void);

#ifdef __cplusplus
}
#endif
