#include "EspLcdDisplay.h"
#include "EspLcdNativeDisplay.h"

#include <assert.h>
#include <esp_lvgl_port_disp.h>
#include <Tactility/Check.h>
#include <Tactility/LogEsp.h>
#include <Tactility/hal/touch/TouchDevice.h>

constexpr char* TAG = "EspLcdDisplay";

EspLcdDisplay::~EspLcdDisplay() {
    if (nativeDisplay != nullptr && nativeDisplay.use_count() > 1) {
        tt_crash("NativeDisplay is still in use. This will cause memory access violations.");
    }
}

bool EspLcdDisplay::start() {
    if (!createIoHandle(ioHandle)) {
        TT_LOG_E(TAG, "Failed to create IO handle");
        return false;
    }

    if (!createPanelHandle(ioHandle, panelHandle)) {
        TT_LOG_E(TAG, "Failed to create panel handle");
        esp_lcd_panel_io_del(ioHandle);
        return false;
    }

    return true;
}

bool EspLcdDisplay::stop() {
    if (lvglDisplay != nullptr) {
        stopLvgl();
        lvglDisplay = nullptr;
    }

    if (panelHandle != nullptr && esp_lcd_panel_del(panelHandle) != ESP_OK) {
        return false;
    }

    if (ioHandle != nullptr && esp_lcd_panel_io_del(ioHandle) != ESP_OK) {
        return false;
    }

    if (nativeDisplay != nullptr && nativeDisplay.use_count() > 1) {
        TT_LOG_W(TAG, "NativeDisplay is still in use.");
    }

    return true;
}

bool EspLcdDisplay::startLvgl() {
    assert(lvglDisplay == nullptr);

    if (nativeDisplay != nullptr && nativeDisplay.use_count() > 1) {
        TT_LOG_W(TAG, "NativeDisplay is still in use.");
    }

    lvglPortDisplayConfig = getLvglPortDisplayConfig(ioHandle, panelHandle);
    lvglDisplay = lvgl_port_add_disp(&lvglPortDisplayConfig);

    auto touch_device = getTouchDevice();
    if (touch_device != nullptr) {
        touch_device->startLvgl(lvglDisplay);
    }

    return lvglDisplay != nullptr;
}

bool EspLcdDisplay::stopLvgl() {
    if (lvglDisplay == nullptr) {
        return false;
    }

    auto touch_device = getTouchDevice();
    if (touch_device != nullptr) {
        touch_device->stopLvgl();
    }

    lvgl_port_remove_disp(lvglDisplay);
    lvglDisplay = nullptr;
    return true;
}

std::shared_ptr<display::NativeDisplay> EspLcdDisplay::getNativeDisplay() {
    assert(lvglDisplay == nullptr); // Still attached to LVGL context. Call stopLvgl() first.
    if (nativeDisplay == nullptr) {
        nativeDisplay = std::make_shared<EspLcdNativeDisplay>(
            panelHandle,
            lvglPortDisplayConfig
        );
    }
    return nativeDisplay;
}
