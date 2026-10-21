// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <tactility/freertos/task.h>
#include <tactility/log.h>

#define TAG "freertos"

/**
 * Assert implementation as defined in FreeRTOSConfig.h
 */
void vAssertCalled(unsigned long line, const char* const file) {
    LOG_E("Assert triggered at {}:{}", file, line);
    abort();
}
