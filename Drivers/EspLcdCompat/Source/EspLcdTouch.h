#pragma once

#include <esp_lcd_touch.h>
#include <esp_lcd_types.h>
#include <lvgl.h>
#include <Tactility/hal/touch/TouchDevice.h>

class EspLcdTouch : public tt::hal::touch::TouchDevice {

    esp_lcd_touch_config_t config;
    esp_lcd_panel_io_handle_t _Nullable ioHandle = nullptr;
    esp_lcd_touch_handle_t _Nullable touchHandle = nullptr;
    lv_indev_t* _Nullable lvglDevice = nullptr;

    void cleanup();

protected:

    virtual bool createIoHandle(esp_lcd_panel_io_handle_t& ioHandle) = 0;

    virtual bool createTouchHandle(esp_lcd_panel_io_handle_t ioHandle, const esp_lcd_touch_config_t& configuration, esp_lcd_touch_handle_t& touchHandle) = 0;

    virtual esp_lcd_touch_config_t createEspLcdTouchConfig() = 0;

public:


    bool start(lv_display_t* display) override;

    bool stop() override;


    lv_indev_t* _Nullable getLvglIndev() override { return lvglDevice; }
};
