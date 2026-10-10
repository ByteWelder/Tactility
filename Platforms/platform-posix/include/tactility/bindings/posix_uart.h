// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/bindings/bindings.h>
#include <tactility/drivers/posix_uart.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_DEVICETREE(posix_uart, struct PosixUartConfig)

#ifdef __cplusplus
}
#endif
