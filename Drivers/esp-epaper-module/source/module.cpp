// SPDX-License-Identifier: Apache-2.0
#include <tactility/driver.h>
#include <tactility/module.h>

extern Driver esp_epaper_driver;

extern "C" {

static Driver* const esp_epaper_drivers[] = {
    &esp_epaper_driver,
    nullptr
};

Module esp_epaper_module = {
    .name = "esp_epaper",
    .drivers = esp_epaper_drivers
};

} // extern "C"
