#pragma once

#include <Tactility/hal/display/DisplayDevice.h>

#include <esp_lcd_types.h>
#include <esp_lvgl_port_disp.h>
#include <Tactility/hal/display/NativeDisplay.h>

class EspLcdDisplay : tt::hal::display::DisplayDevice {

    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* lvglDisplay = nullptr;
    lvgl_port_display_cfg_t lvglPortDisplayConfig;
    std::shared_ptr<tt::hal::display::NativeDisplay> nativeDisplay;

protected:

    // Used for sending commands such as setting curves
    esp_lcd_panel_io_handle_t getIoHandle() const { return ioHandle; }

    virtual bool createIoHandle(esp_lcd_panel_io_handle_t& outHandle) = 0;

    virtual bool createPanelHandle(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t& panelHandle) = 0;

    virtual lvgl_port_display_cfg_t getLvglPortDisplayConfig(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t panelHandle) = 0;

public:

    ~EspLcdDisplay() override;

    bool start() final;

    bool stop() final;

    // region LVGL

    bool supportsLvgl() const final { return true; }

    bool startLvgl() final;

    bool stopLvgl() final;

    lv_display_t* _Nullable getLvglDisplay() const final { return lvglDisplay; }

    // endregion

    // region NativedDisplay

    /** @return a NativeDisplay instance if this device supports it */
    std::shared_ptr<tt::hal::display::NativeDisplay> _Nullable getNativeDisplay() final;

    // endregion
};
