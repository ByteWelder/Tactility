// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/drivers/i2c_controller.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Requires Linux i2c-dev. Starting the device fails with ERROR_NOT_SUPPORTED on other systems. */
struct PosixI2cConfig {
    const char* path;
};

#ifdef __cplusplus
}
#endif
