// SPDX-License-Identifier: Apache-2.0
#include <tactility/module.h>

error_t platform_posix_start_partitions();

extern "C" {

extern Driver posix_uart_driver;
extern Driver posix_i2c_driver;

static Driver* const platform_posix_drivers[] = {
    &posix_uart_driver,
    &posix_i2c_driver,
    nullptr
};

Module platform_posix_module = {
    .name = "platform-posix",
    .start = platform_posix_start_partitions,
    .stop = nullptr,
    .drivers = platform_posix_drivers,
    .symbols = nullptr,
    .internal = nullptr,
};

}
