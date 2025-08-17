#pragma once

#include "tt_hal_device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* TouchDriverHandle;
/**
 * Check if the touch driver interface is supported for this device.
 * @param[in] touchDeviceId the identifier of the touch device
 * @return true if the driver is supported.
 */
bool tt_hal_touch_driver_supported(DeviceId touchDeviceId);

/**
 * Allocate a driver object for the specified touchDeviceId.
 * @warning check whether the driver is supported by calling tt_hal_touch_driver_supported() first
 * @param[in] touchDeviceId the identifier of the touch device
 * @return the driver handle
 */
TouchDriverHandle tt_hal_touch_driver_alloc(DeviceId touchDeviceId);

/**
 * Free the memory for the touch driver.
 * @param[in] handle the touch driver handle
 */
void tt_hal_touch_driver_free(TouchDriverHandle handle);

/**
 * Get the coordinates for the currently touched points on the screen.
 *
 * @param[in] handle the touch driver handle
 * @param[in] x array of X coordinates
 * @param[in] y array of Y coordinates
 * @param[in] strength array of strengths (with the minimum size of maxPointCount) or NULL
 * @param[in] pointCount the number of points currently touched on the screen
 * @param[in] maxPointCount the maximum number of points that can be touched at once
 *
 * @return true when touched and coordinates are available
 */
bool tt_hal_touch_driver_get_touched_points(TouchDriverHandle handle, uint16_t* x, uint16_t* y, uint16_t* strength, uint8_t* pointCount, uint8_t maxPointCount);

#ifdef __cplusplus
}
#endif
