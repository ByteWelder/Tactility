// SPDX-License-Identifier: Apache-2.0
#include <tactility/module.h>

extern "C" {

extern Driver posix_wifi_driver;

static Driver* const platform_posix_drivers[] = {
    &posix_wifi_driver,
    nullptr
};

Module platform_posix_module = {
    .name = "platform-posix",
    .start = nullptr,
    .stop = nullptr,
    .drivers = platform_posix_drivers,
    .symbols = nullptr,
    .internal = nullptr,
};

}
