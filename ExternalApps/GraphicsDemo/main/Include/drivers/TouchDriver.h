#pragma once

#include <cassert>
#include <tt_hal_touch.h>

/**
 * Wrapper for tt_hal_touch_driver_*
 */
class TouchDriver {

    TouchDriverHandle handle = nullptr;

public:

    explicit TouchDriver(DeviceId id) {
        assert(tt_hal_touch_driver_supported(id));
        handle = tt_hal_touch_driver_alloc(id);
        assert(handle != nullptr);
    }

    ~TouchDriver() {
        tt_hal_touch_driver_free(handle);
    }

    bool getTouchedPoints(uint16_t* x, uint16_t* y, uint16_t* strength, uint8_t* count, uint8_t maxCount) const {
        return tt_hal_touch_driver_get_touched_points(handle, x, y, strength, count, maxCount);
    }
};
