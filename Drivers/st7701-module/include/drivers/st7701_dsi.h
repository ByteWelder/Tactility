// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <tactility/device.h>
#include <tactility/drivers/gpio.h>

struct St7701DsiConfig {
    uint16_t horizontal_resolution;
    uint16_t vertical_resolution;
    uint8_t bits_per_pixel;
    bool bgr_order;
    bool invert_color;
    bool mirror_x;
    bool mirror_y;

    struct GpioPinSpec pin_reset;

    int32_t ldo_channel;
    int32_t ldo_voltage_mv;

    uint8_t dsi_bus_id;
    uint8_t num_data_lanes;
    uint32_t lane_bit_rate_mbps;

    uint32_t dpi_clock_freq_mhz;
    uint32_t hsync_pulse_width;
    uint32_t hsync_back_porch;
    uint32_t hsync_front_porch;
    uint32_t vsync_pulse_width;
    uint32_t vsync_back_porch;
    uint32_t vsync_front_porch;
    uint8_t num_fbs;
    bool use_dma2d;
    bool disable_lp;

    bool allow_tearing;

    const uint8_t* init_sequence;
    uint32_t init_sequence_length;

    struct Device* backlight;
};

#ifdef __cplusplus
}
#endif
