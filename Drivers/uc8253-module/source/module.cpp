// SPDX-License-Identifier: Apache-2.0
#include <tactility/driver.h>
#include <tactility/module.h>

extern Driver uc8253_driver;

extern "C" {

static Driver* const uc8253_drivers[] = {
    &uc8253_driver,
    nullptr
};

Module uc8253_module = {
    .name = "uc8253",
    .start = nullptr,
    .stop = nullptr,
    .drivers = uc8253_drivers,
    .symbols = nullptr,
    .internal = nullptr,
};

} // extern "C"
