// SPDX-License-Identifier: Apache-2.0
#include <tactility/drivers/software_pixel_mapper.h>

// RGB332 packs each pixel into one byte: 3 bits red, 3 bits green, 2 bits blue. The byte layout
// maps straight onto an 8-data-line panel's significant color inputs (R7..R5, G7..G5, B7..B6),
// so each RGB565 channel's top bits land on the corresponding MSB lines
static void rgb332_map(SoftwarePixelMapperData data, const uint16_t* src, uint8_t* dst, uint32_t pixel_count) {
    (void)data;
    for (uint32_t i = 0; i < pixel_count; i++) {
        uint16_t px = src[i];
        dst[i] = (uint8_t)(((px >> 13) & 0x07) << 5) | (((px >> 8) & 0x07) << 2) | ((px >> 3) & 0x03);
    }
}

static uint8_t rgb332_instance;

static SoftwarePixelMapperData rgb332_create(uint16_t width, uint16_t height) {
    (void)width;
    (void)height;
    return &rgb332_instance;
}

static void rgb332_destroy(SoftwarePixelMapperData data) {
    (void)data;
}

const struct SoftwarePixelMapper software_pixel_mapper_rgb332 = {
    .create = rgb332_create,
    .map = rgb332_map,
    .destroy = rgb332_destroy,
};
