// SPDX-License-Identifier: Apache-2.0
#include <tactility/driver.h>
#include <tactility/module.h>

extern Driver ssd16xx_driver;

extern "C" {

static Driver* const ssd16xx_drivers[] = {
    &ssd16xx_driver,
    nullptr
};

Module ssd16xx_module = {
    .name = "ssd16xx",
    .start = nullptr,
    .stop = nullptr,
    .drivers = ssd16xx_drivers,
    .symbols = nullptr,
    .internal = nullptr,
};

} // extern "C"
