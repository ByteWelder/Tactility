// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdint.h>
#include <tactility/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Field order must match the property order in bindings/ti,cc1101.yaml */
struct Cc1101Config {
    /** Chip-select index on the parent SPI controller, must match the node's unit address */
    int32_t reg;
    /** SPI clock frequency in kHz */
    uint32_t spi_frequency_khz;
    /** GDO0 interrupt line (must be an SoC GPIO) */
    struct GpioPinSpec pin_gdo0;
    /** Optional RF band switch line 0, driven from bit 0 of the active band's value */
    struct GpioPinSpec pin_band_switch_0;
    /** Optional RF band switch line 1, driven from bit 1 of the active band's value */
    struct GpioPinSpec pin_band_switch_1;
    /** Band switch value for the 300-348 MHz band */
    uint32_t band_switch_low;
    /** Band switch value for the 387-464 MHz band */
    uint32_t band_switch_mid;
    /** Band switch value for the 779-928 MHz band */
    uint32_t band_switch_high;
};

#ifdef __cplusplus
}
#endif
