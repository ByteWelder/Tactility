// SPDX-License-Identifier: Apache-2.0
#include <tactility/driver.h>
#include <tactility/module.h>

extern "C" {

extern Driver sh8601_driver;
extern Driver sh8601_backlight_driver;

static Driver* const sh8601_drivers[] = {
    &sh8601_driver,
    &sh8601_backlight_driver,
    nullptr
};

Module sh8601_module = {
    .name = "sh8601",
    .start = nullptr,
    .stop = nullptr,
    .drivers = sh8601_drivers,
    .symbols = nullptr,
    .internal = nullptr
};

} // extern "C"
