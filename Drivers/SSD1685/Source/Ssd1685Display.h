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

        /* Physical panel dimensions (portrait native, before any rotation).
         * width  = source direction (168 for GDEY029T71H)
         * height = gate direction  (384 for GDEY029T71H) */
        uint16_t width, height;

        /* rotation: 0 = portrait, 1 = landscape 90 CW, 2 = 180, 3 = landscape 90 CCW.
         * Implemented via LVGL software rotation. The hardware always runs in
         * portrait mode (no swap_xy / mirror on the panel). */
        uint8_t rotation = 0;

        /* gapX: source-line hardware offset (S0..S(gapX-1) unconnected).
         * For GDEY029T71H: gapX=8 (S8..S175 connected). */
        int gapX = 0;
        int gapY = 0;

        uint32_t spiClockHz    = 4'000'000;
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

    /* L8 draw buffer: physical portrait pixels, 1 byte each.
     * Size = width * height bytes.  LVGL sw-rotates into this before flush. */
    uint8_t* drawBuf = nullptr;

    /* 1bpp packed buffer: L8 thresholded and packed for the SSD1685 RAM.
     * Size = ((width - gapX + 7) / 8) * height bytes. */
    uint8_t* packedBuf = nullptr;

    bool started = false;

    SemaphoreHandle_t refreshSemaphore  = nullptr;
    TaskHandle_t      refreshTaskHandle = nullptr;

    /* Physical portrait dimensions reported to LVGL (no gap subtraction needed
     * because we always operate in portrait mode; the gap is applied purely at
     * the RAM window level inside draw_bitmap). */
    uint16_t physWidth()  const { return config->width; }
    uint16_t physHeight() const { return config->height; }

    /* LVGL rotation enum: maps config->rotation to lv_display_rotation_t.
     * The hardware never changes orientation; LVGL sw-rotates to taste. */
    static lv_display_rotation_t lvglRotation(uint8_t rotation);

    static void flushCallback(lv_display_t* disp,
                              const lv_area_t* area,
                              uint8_t* pixelMap);
    static void epdRefreshTask(void* params);

public:

    explicit Ssd1685Display(std::unique_ptr<Configuration> cfg);
    ~Ssd1685Display() override;

    std::string getName()        const override { return "SSD1685"; }
    std::string getDescription() const override { return "SSD168x e-paper display (L8)"; }

    bool start()  override;
    bool stop()   override;

    bool supportsPowerControl() const override { return false; }
    bool isPoweredOn()          const override { return started; }
    void setPowerOn(bool)             override {}

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
