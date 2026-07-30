// SPDX-License-Identifier: Apache-2.0
#include <tactility/driver.h>
#include <tactility/module.h>

extern "C" {

extern Driver gpio_trackball_driver;

static Driver* const gpio_trackball_drivers[] = {
    &gpio_trackball_driver,
    nullptr
};

Module gpio_trackball_module = {
    .name = "gpio-trackball",
    .drivers = gpio_trackball_drivers
};

} // extern "C"
