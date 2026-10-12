// SPDX-License-Identifier: Apache-2.0
#include <tactility/driver.h>
#include <tactility/module.h>

extern "C" {

extern Driver cc1101_driver;

static Driver* const cc1101_drivers[] = {
    &cc1101_driver,
    nullptr
};

Module cc1101_module = {
    .name = "cc1101",
    .start = nullptr,
    .stop = nullptr,
    .drivers = cc1101_drivers,
    .symbols = nullptr,
    .internal = nullptr
};

} // extern "C"
