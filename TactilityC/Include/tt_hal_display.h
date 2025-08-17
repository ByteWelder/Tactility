#pragma once

#include "tt_hal_device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* DisplayDriverHandle;

enum ColorFormat {
    COLOR_FORMAT_MONOCHROME, // 1 bpp
    COLOR_FORMAT_BGR565,
    COLOR_FORMAT_BGR565_SWAPPED,
    COLOR_FORMAT_RGB565,
    COLOR_FORMAT_RGB565_SWAPPED,
    COLOR_FORMAT_RGB888
};

bool tt_hal_display_driver_supported(DeviceId id);

DisplayDriverHandle tt_hal_display_driver_alloc(DeviceId id);

void tt_hal_display_driver_free(DisplayDriverHandle handle);

ColorFormat tt_hal_display_driver_get_colorformat(DisplayDriverHandle handle);

uint16_t tt_hal_display_driver_get_pixel_width(DisplayDriverHandle handle);

uint16_t tt_hal_display_driver_get_pixel_height(DisplayDriverHandle handle);

void tt_hal_display_driver_draw_bitmap(DisplayDriverHandle handle, int xStart, int yStart, int xEnd, int yEnd, const void* pixelData);

#ifdef __cplusplus
}
#endif
