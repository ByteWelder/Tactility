// SPDX-License-Identifier: Apache-2.0
#include <tactility/driver.h>
#include <tactility/module.h>

extern "C" {

#include <soc/soc_caps.h>

extern Driver st7701_driver;
#if SOC_MIPI_DSI_SUPPORTED
extern Driver st7701_dsi_driver;
#endif

static Driver* const st7701_drivers[] = {
    &st7701_driver,
#if SOC_MIPI_DSI_SUPPORTED
    &st7701_dsi_driver,
#endif
    nullptr
};

Module st7701_module = {
    .name = "st7701",
    .start = nullptr,
    .stop = nullptr,
    .drivers = st7701_drivers,
    .symbols = nullptr,
    .internal = nullptr
};

} // extern "C"
