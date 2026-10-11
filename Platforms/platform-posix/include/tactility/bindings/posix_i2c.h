// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/bindings/bindings.h>
#include <tactility/drivers/posix_i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_DEVICETREE(posix_i2c, struct PosixI2cConfig)

#ifdef __cplusplus
}
#endif
