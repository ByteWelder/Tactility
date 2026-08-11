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

/**
 * Rotates one display-space tile into the native shadow frame at its rotated
 * position, leaving all pixels outside the tile untouched. The pixel mapping is
 * identical to esp_epaper_rotate_frame() restricted to the tile, so a shadow
 * accumulated tile-by-tile equals the full-frame rotation of the same content.
 *
 * Unlike esp_epaper_rotate_frame() - which seeds a scratch buffer black and
 * only ORs in set (white) bits - this writes every pixel of the tile with both
 * polarities, because the shadow persists across cycles: it mirrors the cleared
 * (all-white) GDDRAM, so white pixels keep the seed while black pixels must
 * clear their bit. A tile never overlaps itself, so unconditional writes are
 * safe; outside the tile the shadow is untouched.
 *
 * width/height are the panel's native dimensions. (x1,y1,x2,y2) is the
 * display-space tile rectangle with exclusive x2/y2. src holds the tile's
 * pixels (stride = ceil((x2 - x1) / 8) bytes, MSB-first bits). dst is the
 * native full frame (stride = ceil(width / 8) bytes); it may not alias src.
 */
static inline void esp_epaper_rotate_tile(const uint8_t* src, uint8_t* dst, uint16_t width, uint16_t height,
                                          uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t rotation) {
    const uint32_t src_stride = ((uint32_t)(x2 - x1) + 7) / 8;
    const uint32_t dst_stride = ((uint32_t)width + 7) / 8;

    for (uint16_t v = y1; v < y2; v++) {
        for (uint16_t u = x1; u < x2; u++) {
            uint16_t x;
            uint16_t y;
            switch (rotation) {
                case 0:
                    x = u;
                    y = v;
                    break;
                case 1:
                    x = v;
                    y = height - 1 - u;
                    break;
                case 2:
                    x = width - 1 - u;
                    y = height - 1 - v;
                    break;
                case 3:
                default:
                    x = width - 1 - v;
                    y = u;
                    break;
            }
            const uint8_t src_bit = 0x80 >> ((u - x1) % 8);
            const uint8_t dst_mask = 0x80 >> (x % 8);
            uint8_t* const dst_byte = &dst[(uint32_t)y * dst_stride + x / 8];
            if (src[(uint32_t)(v - y1) * src_stride + (u - x1) / 8] & src_bit) {
                *dst_byte |= dst_mask;
            } else {
                *dst_byte &= (uint8_t)~dst_mask;
            }
        }
    }
}

/**
 * Computes the native-space rectangle a display-space tile covers, using the
 * same inverse-of-LVGL mapping as esp_epaper_rotate_frame(). The tile is
 * (x1,y1,x2,y2) in display coordinates (exclusive x2/y2) with width/height the
 * panel's native dimensions; the native rect is written to out_x/out_y/out_w/
 * out_h (out_w/out_h exclusive extents).
 */
static inline void esp_epaper_rotate_tile_rect(uint16_t width, uint16_t height,
                                               uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t rotation,
                                               uint16_t* out_x, uint16_t* out_y, uint16_t* out_w, uint16_t* out_h) {
    uint16_t nx1;
    uint16_t ny1;
    uint16_t nx2;
    uint16_t ny2;
    switch (rotation) {
        case 0:
            nx1 = x1;
            ny1 = y1;
            nx2 = x2;
            ny2 = y2;
            break;
        case 1:
            nx1 = y1;
            ny1 = height - x2;
            nx2 = y2;
            ny2 = height - x1;
            break;
        case 2:
            nx1 = width - x2;
            ny1 = height - y2;
            nx2 = width - x1;
            ny2 = height - y1;
            break;
        case 3:
        default:
            nx1 = width - y2;
            ny1 = x1;
            nx2 = width - y1;
            ny2 = x2;
            break;
    }
    *out_x = nx1;
    *out_y = ny1;
    *out_w = nx2 - nx1;
    *out_h = ny2 - ny1;
}

#ifdef __cplusplus
}
#endif
