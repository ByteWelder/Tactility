#pragma once

#include <Tactility/Mutex.h>

#include <EspLcdDisplay.h>
#include <lvgl.h>

class St7701Display final : public EspLcdDisplay {

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable touchDevice;

    bool createIoHandle(esp_lcd_panel_io_handle_t& outHandle) override;

    bool createPanelHandle(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t& panelHandle) override;

    lvgl_port_display_cfg_t getLvglPortDisplayConfig(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t panelHandle) override;

    bool isRgbPanel() const override { return true; }

    lvgl_port_display_rgb_cfg_t getLvglPortDisplayRgbConfig(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t panelHandle) override;

public:

    St7701Display() : EspLcdDisplay(std::make_shared<tt::Mutex>(tt::Mutex::Type::Recursive)) {}

    std::string getName() const override { return "ST7701S"; }

    std::string getDescription() const override { return "ST7701S RGB display"; }

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable getTouchDevice() override;

    void setBacklightDuty(uint8_t backlightDuty) override;

    bool supportsBacklightDuty() const override { return true; }
};
