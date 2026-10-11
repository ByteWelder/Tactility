// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/drivers/uart_controller.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PosixUartConfig {
    const char* path;
};

#ifdef __cplusplus
}
#endif
