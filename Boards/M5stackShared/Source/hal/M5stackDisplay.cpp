#include "M5stackDisplay.h"
#include "M5stackTouch.h"
#include "Log.h"

#include <TactilityCore.h>

#define TAG "m5shared_display"


#include "M5Unified.hpp"
#include "esp_err.h"
#include "esp_lvgl_port.h"

static void flush_callback(lv_display_t* display, const lv_area_t* area, uint8_t* px_map) {
    M5GFX& gfx = *(M5GFX*)lv_display_get_driver_data(display);

    int32_t width = (area->x2 - area->x1 + 1);
    int32_t height = (area->y2 - area->y1 + 1);

    gfx.startWrite();
    gfx.setAddrWindow(area->x1, area->y1, width, height);
    gfx.writePixels((lgfx::rgb565_t*)px_map, width * height);
    gfx.endWrite();

    lv_display_flush_ready(display);
}

_Nullable lv_disp_t* createDisplay(bool usePsram) {
    M5GFX& gfx = M5.Display;

    static lv_display_t* display = lv_display_create(gfx.width(), gfx.height());
    LV_ASSERT_MALLOC(display)
    if (display == nullptr) {
        return nullptr;
    }

    lv_display_set_driver_data(display, &gfx);
    lv_display_set_flush_cb(display, flush_callback);

    const size_t bytes_per_pixel = 2; // assume 16-bit color
    const size_t buffer_size = gfx.width() * gfx.height() * bytes_per_pixel / 8;
    if (usePsram) {
        static auto* buffer1 = (uint8_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
        LV_ASSERT_MALLOC(buffer1)
        static auto* buffer2 = (uint8_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
        LV_ASSERT_MALLOC(buffer2)
        lv_display_set_buffers(
            display,
            (void*)buffer1,
            (void*)buffer2,
            buffer_size,
            LV_DISPLAY_RENDER_MODE_PARTIAL
        );
    } else {
        static auto* buffer = (uint8_t*)malloc(buffer_size);
        LV_ASSERT_MALLOC(buffer)
        lv_display_set_buffers(
            display,
            (void*)buffer,
            nullptr,
            buffer_size,
            LV_DISPLAY_RENDER_MODE_PARTIAL
        );
    }

    return display;
}

bool M5stackDisplay::start() {
    TT_LOG_I(TAG, "Starting");
    displayHandle = createDisplay(true);
    TT_LOG_I(TAG, "Finished");
    return displayHandle != nullptr;
}

bool M5stackDisplay::stop() {
    tt_assert(displayHandle != nullptr);
    lv_display_delete(displayHandle);
    displayHandle = nullptr;
    return true;
}

tt::hal::Touch* _Nullable M5stackDisplay::createTouch() {
    return static_cast<tt::hal::Touch*>(new M5stackTouch());
}

tt::hal::Display* createDisplay() {
    return static_cast<tt::hal::Display*>(new M5stackDisplay());
}
