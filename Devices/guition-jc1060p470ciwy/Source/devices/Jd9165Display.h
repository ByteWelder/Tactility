#pragma once

#include <Tactility/RecursiveMutex.h>
#include <EspLcdDisplayV2.h>

#include <esp_lcd_mipi_dsi.h>

class Jd9165Display final : public EspLcdDisplayV2 {

    class NoLock final : public tt::Lock {
        bool lock(TickType_t timeout) const override { return true; }
        void unlock() const override { /* NO-OP */ }
    };

    esp_lcd_dsi_bus_handle_t mipiDsiBus = nullptr;

    bool createMipiDsiBus();

protected:

    bool createIoHandle(esp_lcd_panel_io_handle_t& ioHandle) override;

    esp_lcd_panel_dev_config_t createPanelConfig(std::shared_ptr<EspLcdConfiguration> espLcdConfiguration, gpio_num_t resetPin) override;

    bool createPanelHandle(esp_lcd_panel_io_handle_t ioHandle, const esp_lcd_panel_dev_config_t& panelConfig, esp_lcd_panel_handle_t& panelHandle) override;

    bool useDsiPanel() const override { return true; }

    lvgl_port_display_dsi_cfg_t getLvglPortDisplayDsiConfig(esp_lcd_panel_io_handle_t /*ioHandle*/, esp_lcd_panel_handle_t /*panelHandle*/) override;

public:

    Jd9165Display(
        const std::shared_ptr<EspLcdConfiguration>& configuration
    ) : EspLcdDisplayV2(configuration, std::make_shared<NoLock>()) {}

    ~Jd9165Display() override;

    std::string getName() const override { return "JD9165"; }

    std::string getDescription() const override { return "JD9165 MIPI-DSI 1024x600 display"; }
};
