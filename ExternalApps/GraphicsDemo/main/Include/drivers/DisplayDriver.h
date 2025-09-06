#pragma once

#include <cassert>
#include <tt_hal_display.h>

/**
 * Wrapper for tt_hal_display_driver_*
 */
class DisplayDriver {

    DisplayDriverHandle handle = nullptr;

public:

    explicit DisplayDriver(DeviceId id) {
        assert(tt_hal_display_driver_supported(id));
        handle = tt_hal_display_driver_alloc(id);
        assert(handle != nullptr);
    }

    ~DisplayDriver() {
        tt_hal_display_driver_free(handle);
    }

    bool lock(TickType timeout = TT_MAX_TICKS) const {
        return tt_hal_display_driver_lock(handle, timeout);
    }

    void unlock() const {
        tt_hal_display_driver_unlock(handle);
    }

    uint16_t getWidth() const {
        return tt_hal_display_driver_get_pixel_width(handle);
    }

    uint16_t getHeight() const {
        return tt_hal_display_driver_get_pixel_height(handle);
    }

    ColorFormat getColorFormat() const {
        return tt_hal_display_driver_get_colorformat(handle);
    }

    void drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) const {
        tt_hal_display_driver_draw_bitmap(handle, xStart, yStart, xEnd, yEnd, pixelData);
    }
};
