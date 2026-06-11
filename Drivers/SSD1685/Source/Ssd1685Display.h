#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/touch/TouchDevice.h>

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_ssd1685.h>
#include <driver/spi_common.h>
#include <driver/gpio.h>

#include <memory>
#include <string>

class Ssd1685Display final : public tt::hal::display::DisplayDevice {

public:

    struct Configuration {

        Configuration(
            spi_host_device_t spiHost,
            gpio_num_t        csPin,
            gpio_num_t        dcPin,
            gpio_num_t        resetPin,
            gpio_num_t        busyPin,
            uint16_t          width,
            uint16_t          height,
            uint8_t           rotation = 0,
            std::shared_ptr<tt::hal::touch::TouchDevice> touch = nullptr
        ) :
            spiHost(spiHost), csPin(csPin), dcPin(dcPin),
            resetPin(resetPin), busyPin(busyPin),
            width(width), height(height),
            rotation(rotation), touch(std::move(touch))
        {}

        spi_host_device_t spiHost;
        gpio_num_t csPin, dcPin, resetPin, busyPin;
        uint16_t   width, height;

        uint8_t rotation = 0;
        int gapX = 0, gapY = 0;
        uint32_t spiClockHz = 4'000'000;
        uint32_t busyTimeoutMs = 10'000;
        ssd1685_refresh_mode_t refreshMode = SSD1685_REFRESH_FULL;
        const uint8_t* customLut     = nullptr;
        size_t         customLutSize = 0;

        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
    };

private:

    std::unique_ptr<Configuration> config;

    esp_lcd_panel_io_handle_t  ioHandle    = nullptr;
    esp_lcd_panel_handle_t     panelHandle = nullptr;
    lv_display_t*              lvglDisplay = nullptr;

    uint8_t*  drawBuf[2] = {nullptr, nullptr};
    size_t    bufSize = 0;

    /**
     * Repack buffer: LVGL I1 rows are padded to 4-byte alignment but the
     * SSD1685 driver expects 1-byte-aligned rows.  This buffer holds the
     * repacked data at EPD stride before being handed to draw_bitmap.
     *
     * Size = ((lvglWidth + 7) / 8) * lvglHeight  bytes.
     */
    uint8_t*  repackBuf = nullptr;

    bool started = false;

    SemaphoreHandle_t refreshSemaphore = nullptr;
    TaskHandle_t      refreshTaskHandle = nullptr;

    /**
     * lvglWidth / lvglHeight now subtract the gap from the
     * correct axis so LVGL never addresses dead source columns.
     *
     * For rotation 0/2 (portrait / 180):
     *   LVGL width  = panel width  - gapX   (gapX is in the source/X direction)
     *   LVGL height = panel height - gapY
     *
     * For rotation 1/3 (landscape CW/CCW, swap_xy active):
     *   Physical X (source) becomes logical Y after the axis swap.
     *   So gapX shrinks the logical height, and gapY shrinks the logical width.
     */
    uint16_t lvglWidth()  const {
        return (config->rotation == 1 || config->rotation == 3)
               ? config->height - config->gapY
               : config->width  - config->gapX;
    }

    uint16_t lvglHeight() const {
        return (config->rotation == 1 || config->rotation == 3)
               ? config->width  - config->gapX
               : config->height - config->gapY;
    }

    esp_err_t applyRotation();

    static void flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* pixelMap);
    static void epdRefreshTask(void* params);

public:

    explicit Ssd1685Display(std::unique_ptr<Configuration> cfg);
    ~Ssd1685Display() override;

    std::string getName()        const override { return "SSD1685"; }
    std::string getDescription() const override { return "SSD168x e-paper display"; }

    bool start()  override;
    bool stop()   override;

    bool supportsPowerControl()  const override { return false; }
    bool isPoweredOn()           const override { return started; }
    void setPowerOn(bool)              override {}

    std::shared_ptr<tt::hal::touch::TouchDevice> getTouchDevice() override {
        return config->touch;
    }

    bool          supportsLvgl()   const override { return true; }
    bool          startLvgl()            override;
    bool          stopLvgl()             override;
    lv_display_t* getLvglDisplay() const override { return lvglDisplay; }

    bool supportsDisplayDriver()   const override { return false; }
    std::shared_ptr<tt::hal::display::DisplayDriver> getDisplayDriver() override {
        return nullptr;
    }

    esp_err_t clearScreen(uint8_t colorByte = 0xFF);
    esp_err_t sleep();
};
