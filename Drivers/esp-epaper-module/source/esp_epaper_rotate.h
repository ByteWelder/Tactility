// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Rotates a full 1bpp frame (row-major, MSB-first) from the display's rotated
 * space into the panel's native space. The display is the panel's native
 * width x height turned counter-clockwise by `rotation` (0 = 0, 1 = 90, 2 = 180,
 * 3 = 270 degrees), so its own dimensions are the swapped native ones for
 * 90/270. Each mapping is the inverse of LVGL's lv_display_rotate_area()
 * (Libraries/lvgl/src/display/lv_display.c), i.e. the exact transform LVGL
 * applies when displaying on the panel.
 *
 * src points to the display frame (stride = ceil(display_width / 8) bytes,
 * MSB-first bits), dst to the native frame (stride = ceil(width / 8) bytes).
 * dst is overwritten; it may not alias src.
 */
static inline bool esp_epaper_rotation_swaps_axes(uint8_t rotation) {
    return rotation == 1 || rotation == 3;
}

static inline void esp_epaper_rotate_frame(const uint8_t* src, uint8_t* dst, uint16_t width, uint16_t height, uint8_t rotation) {
    const uint16_t display_width = esp_epaper_rotation_swaps_axes(rotation) ? height : width;
    const uint32_t src_stride = (display_width + 7) / 8;
    const uint32_t dst_stride = (width + 7) / 8;

    memset(dst, 0, dst_stride * height);

    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            // Display pixel (u, v) that lands on native pixel (x, y).
            uint16_t u;
            uint16_t v;
            switch (rotation) {
                case 0:
                    u = x;
                    v = y;
                    break;
                case 1:
                    u = height - 1 - y;
                    v = x;
                    break;
                case 2:
                    u = width - 1 - x;
                    v = height - 1 - y;
                    break;
                case 3:
                default:
                    u = y;
                    v = width - 1 - x;
                    break;
            }
            if (src[(uint32_t)v * src_stride + u / 8] & (0x80 >> (u % 8))) {
                dst[(uint32_t)y * dst_stride + x / 8] |= (0x80 >> (x % 8));
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
