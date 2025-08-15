#include "EspLcdTouch.h"

#include <esp_lvgl_port_touch.h>
#include <Tactility/LogEsp.h>

constexpr char* TAG = "EspLcdTouch";

bool EspLcdTouch::start(lv_display_t* display) {
    if (!createIoHandle(ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Touch IO failed");
        return false;
    }

    config = createEspLcdTouchConfig();

    if (!createTouchHandle(ioHandle, config, touchHandle)) {
        TT_LOG_E(TAG, "Driver init failed");
        cleanup();
        return false;
    }

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = display,
        .handle = touchHandle,
    };

    TT_LOG_I(TAG, "Adding touch to LVGL");
    lvglDevice = lvgl_port_add_touch(&touch_cfg);
    if (lvglDevice == nullptr) {
        TT_LOG_E(TAG, "Adding touch failed");
        cleanup();
        return false;
    }

    return true;
}

bool EspLcdTouch::stop() {
    cleanup();
    return true;
}

void EspLcdTouch::cleanup() {
    if (lvglDevice != nullptr) {
        lv_indev_delete(lvglDevice);
        lvglDevice = nullptr;
        // TODO: is this correct? We don't have to delete it manually?
        touchHandle = nullptr;
    }

    if (ioHandle != nullptr) {
        esp_lcd_panel_io_del(ioHandle);
        ioHandle = nullptr;
    }
}
