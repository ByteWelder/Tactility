// SPDX-License-Identifier: Apache-2.0
#include <tactility/driver.h>
#include <tactility/module.h>

extern "C" {

extern Driver sx1262_driver;

static Driver* const sx126x_drivers[] = {
    &sx1262_driver,
    nullptr
};

Module sx126x_module = {
    .name = "sx126x",
    .drivers = sx126x_drivers
};

} // extern "C"
