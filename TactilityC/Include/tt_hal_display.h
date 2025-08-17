#pragma once

#include <tt_kernel.h>

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

/**
 * Check if the display driver interface is supported for this device.
 * @param[in] displayId the identifier of the display device
 * @return true if the driver is supported.
 */
bool tt_hal_display_driver_supported(DeviceId displayId);

/**
 * Allocate a driver object for the specified displayId.
 * @warning check whether the driver is supported by calling tt_hal_display_driver_supported() first
 * @param[in] displayId the identifier of the display device
 * @return the driver handle
 */
DisplayDriverHandle tt_hal_display_driver_alloc(DeviceId displayId);

/**
 * Free the memory for the display driver.
 * @param[in] handle the display driver handle
 */
void tt_hal_display_driver_free(DisplayDriverHandle handle);

/**
 * Lock the display device. Call this function before doing any draw calls.
 * Certain display devices are on a shared bus (e.g. SPI) so they must run
 * mutually exclusive with other devices on the same bus (e.g. SD card)
 * @param[in] handle the display driver handle
 * @param[in] timeout the maximum amount of ticks to wait for getting a lock
 * @return true if the lock was acquired
 */
bool tt_hal_display_driver_lock(DisplayDriverHandle handle, TickType timeout);

/**
 * Unlock the display device. Must be called exactly once after locking.
 * @param[in] handle the display driver handle
 */
void tt_hal_display_driver_unlock(DisplayDriverHandle handle);

/**
 * @param[in] handle the display driver handle
 * @return the native color format for this display
 */
ColorFormat tt_hal_display_driver_get_colorformat(DisplayDriverHandle handle);

/**
 * @param[in] handle the display driver handle
 * @return the horizontal resolution of the display
 */
uint16_t tt_hal_display_driver_get_pixel_width(DisplayDriverHandle handle);

/**
 * @param[in] handle the display driver handle
 * @return the vertical resolution of the display
 */
uint16_t tt_hal_display_driver_get_pixel_height(DisplayDriverHandle handle);

/**
 * Draw pixels on the screen. Make sure to call the lock function first and unlock afterwards.
 * Many draw calls can be done inbetween a single lock and unlock.
 * @param[in] handle the display driver handle
 * @param[in] xStart the starting x coordinate for rendering the pixel data
 * @param[in] yStart the starting y coordinate for rendering the pixel data
 * @param[in] xEnd the last x coordinate for rendering the pixel data (absolute pixel value, not relative to xStart!)
 * @param[in] yEnd the last y coordinate for rendering the pixel data (absolute pixel value, not relative to yStart!)
 * @param[in] pixelData a buffer of pixels. the data is placed as "RowRowRowRow". The size depends on the ColorFormat
 */
void tt_hal_display_driver_draw_bitmap(DisplayDriverHandle handle, int xStart, int yStart, int xEnd, int yEnd, const void* pixelData);

#ifdef __cplusplus
}
#endif
