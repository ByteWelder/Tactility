// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/bindings/bindings.h>
#include <drivers/cc1101.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_DEVICETREE(cc1101, struct Cc1101Config)

#ifdef __cplusplus
}
#endif
