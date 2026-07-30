// SPDX-License-Identifier: GPL-3.0-or-later
#include <tactility/driver.h>
#include <tactility/module.h>

extern "C" {

extern Driver meshtastic_gps_driver;

static Driver* const meshtastic_generic_drivers[] = {
    &meshtastic_gps_driver,
    nullptr
};

Module gps_generic_module = {
    .name = "gps-generic",
    .drivers = meshtastic_generic_drivers
};

}
