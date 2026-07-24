// SPDX-License-Identifier: GPL-3.0-or-later
#include <tactility/driver.h>
#include <tactility/module.h>

extern "C" {

extern Driver generic_gps_driver;

static Driver* const gps_generic_drivers[] = {
    &generic_gps_driver,
    nullptr
};

Module gps_generic_module = {
    .name = "gps-generic",
    .drivers = gps_generic_drivers
};

}
