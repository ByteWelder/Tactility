#include "Ssd1685Display.h"

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_check.h>

#include <tactility/log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

static constexpr const char* TAG = "Ssd1685Display";

Ssd1685Display::Ssd1685Display(std::unique_ptr<Configuration> cfg)
    : config(std::move(cfg))
{}

Ssd1685Display::~Ssd1685Display()
{
    if (lvglDisplay) stopLvgl();
    if (started)     stop();
}

// EPD refresh task (runs outside LVGL lock)

void Ssd1685Display::epdRefreshTask(void* params)
{
    auto* self = static_cast<Ssd1685Display*>(params);
    while (true) {
        if (xSemaphoreTake(self->refreshSemaphore, portMAX_DELAY) == pdTRUE) {
            esp_lcd_ssd1685_refresh(self->panelHandle, self->config->refreshMode);
        }
    }
}

uint16_t Ssd1685Display::lvglWidth() const
{
    return (config->rotation == 1 || config->rotation == 3)
           ? config->height : config->width;
}

uint16_t Ssd1685Display::lvglHeight() const
{
    return (config->rotation == 1 || config->rotation == 3)
           ? config->width : config->height;
}

esp_err_t Ssd1685Display::applyRotation()
{
    bool swap_xy  = false;
    bool mirror_x = false;
    bool mirror_y = false;

    switch (config->rotation) {
    case 0: break;
    case 1: swap_xy = true;  mirror_x = true;  break;
    case 2: mirror_x = true; mirror_y = true;  break;
    case 3: swap_xy = true;  mirror_y = true;  break;
    default:
        LOG_W(TAG, "Unknown rotation %d, using 0", config->rotation);
        break;
    }
    if (swap_xy)
        ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panelHandle, true), TAG, "swap_xy");
    if (mirror_x || mirror_y)
        ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panelHandle, mirror_x, mirror_y), TAG, "mirror");
    return ESP_OK;
}

// flush callback (async, called by LVGL)
//
// Writes pixel data to EPD RAM (fast, ~ms) and signals a separate task to
// trigger the actual display refresh (~4-5 s for FULL).  The LVGL lock is
// released quickly so the GUI can keep working while the EPD updates.

void Ssd1685Display::flushCallback(lv_display_t* disp,
                                    const lv_area_t* area,
                                    uint8_t* pixelMap)
{
    auto* self = static_cast<Ssd1685Display*>(lv_display_get_user_data(disp));
    if (!self || !self->panelHandle || !pixelMap) {
        lv_display_flush_ready(disp);
        return;
    }

    /* pixelMap is I1 (1 bpp, 8 pixels/byte, MSB = leftmost pixel).
     * area specifies the rendered rectangle within the buffer. */

    /* Write the rendered area to EPD RAM (fast SPI transfer). */
    esp_lcd_panel_draw_bitmap(
        self->panelHandle,
        area->x1, area->y1,
        area->x2 + 1, area->y2 + 1,
        pixelMap);

    /* Signal the background refresh task on the last flush of this frame. */
    if (lv_display_flush_is_last(disp)) {
        xSemaphoreGive(self->refreshSemaphore);
    }

    lv_display_flush_ready(disp);
}

// start / stop

bool Ssd1685Display::start()
{
    if (started) {
        LOG_W(TAG, "Already started");
        return true;
    }

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num         = config->csPin,
        .dc_gpio_num         = config->dcPin,
        .spi_mode            = 0,
        .pclk_hz             = config->spiClockHz,
        .trans_queue_depth   = 4,
        .on_color_trans_done = nullptr,
        .user_ctx            = nullptr,
        .lcd_cmd_bits        = 8,
        .lcd_param_bits      = 8,
        .cs_ena_pretrans     = 0,
        .cs_ena_posttrans    = 0,
        .flags = {
            .dc_high_on_cmd  = 0,
            .dc_low_on_data  = 0,
            .dc_low_on_param = 0,
            .octal_mode      = 0,
            .quad_mode       = 0,
            .sio_mode        = 1,
            .lsb_first       = 0,
            .cs_high_active  = 0,
        }
    };

    if (esp_lcd_new_panel_io_spi(
            (esp_lcd_spi_bus_handle_t)config->spiHost,
            &io_cfg, &ioHandle) != ESP_OK) {
        LOG_E(TAG, "Failed to create SPI panel IO");
        return false;
    }

    esp_lcd_panel_ssd1685_config_t epd_cfg = {
        .busy_gpio_num        = config->busyPin,
        .busy_timeout_ms      = config->busyTimeoutMs,
        .panel_width          = config->width,
        .panel_height         = config->height,
        .non_copy_mode        = true,
        .default_refresh_mode = config->refreshMode,
        .custom_lut           = config->customLut,
        .custom_lut_size      = config->customLutSize,
        .on_reset             = nullptr,
        .on_reset_user_data   = nullptr,
    };

    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = config->resetPin;
    panel_cfg.bits_per_pixel = 1;
    panel_cfg.vendor_config  = &epd_cfg;

    if (esp_lcd_new_panel_ssd1685(ioHandle, &panel_cfg, &panelHandle) != ESP_OK) {
        LOG_E(TAG, "Failed to create SSD1685 panel");
        esp_lcd_panel_io_del(ioHandle);
        ioHandle = nullptr;
        return false;
    }

    if (esp_lcd_panel_reset(panelHandle) != ESP_OK ||
        esp_lcd_panel_init(panelHandle)  != ESP_OK) {
        LOG_E(TAG, "Panel reset/init failed");
        esp_lcd_panel_del(panelHandle);
        esp_lcd_panel_io_del(ioHandle);
        panelHandle = nullptr;
        ioHandle    = nullptr;
        return false;
    }

    if (config->gapX != 0 || config->gapY != 0) {
        esp_lcd_panel_set_gap(panelHandle, config->gapX, config->gapY);
    }
    applyRotation();

    LOG_I(TAG, "Initial clear...");
    esp_lcd_ssd1685_clear(panelHandle, 0xFF);

    started = true;
    LOG_I(TAG, "Started  %dx%d  rotation=%d  gap=(%d,%d)",
          config->width, config->height,
          config->rotation, config->gapX, config->gapY);
    return true;
}

bool Ssd1685Display::stop()
{
    if (!started) return true;
    if (lvglDisplay) stopLvgl();

    esp_lcd_ssd1685_sleep(panelHandle, SSD1685_DEEP_SLEEP_MODE1);
    if (panelHandle) { esp_lcd_panel_del(panelHandle);  panelHandle = nullptr; }
    if (ioHandle)    { esp_lcd_panel_io_del(ioHandle);  ioHandle    = nullptr; }

    started = false;
    LOG_I(TAG, "Stopped");
    return true;
}

// LVGL

bool Ssd1685Display::startLvgl()
{
    if (lvglDisplay) {
        LOG_W(TAG, "LVGL already started");
        return true;
    }
    if (!started) {
        LOG_E(TAG, "Call start() before startLvgl()");
        return false;
    }

    uint16_t w = lvglWidth();
    uint16_t h = lvglHeight();

    /* I1: 1 bit per pixel, row stride = (w + 7) / 8 bytes */
    size_t stride = ((size_t)w + 7U) / 8U;
    bufSize = stride * (size_t)h;

    LOG_I(TAG, "Allocating 2 x I1 buffers  %zu bytes (%dx%d)",
          bufSize, w, h);

    for (int i = 0; i < 2; ++i) {
        drawBuf[i] = static_cast<uint8_t*>(
            heap_caps_malloc(bufSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!drawBuf[i]) {
            drawBuf[i] = static_cast<uint8_t*>(
                heap_caps_malloc(bufSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
        }
        if (!drawBuf[i]) {
            LOG_E(TAG, "Failed to allocate I1 buffer %d", i);
            for (int j = 0; j < i; ++j) {
                heap_caps_free(drawBuf[j]); drawBuf[j] = nullptr;
            }
            return false;
        }
        memset(drawBuf[i], 0xFF, bufSize);
    }

    lvglDisplay = lv_display_create(w, h);
    if (!lvglDisplay) {
        LOG_E(TAG, "lv_display_create failed");
        for (auto& buf : drawBuf) {
            heap_caps_free(buf);
            buf = nullptr;
        }
        return false;
    }

    lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_I1);

    lv_display_set_buffers(
        lvglDisplay,
        drawBuf[0], drawBuf[1],
        bufSize,
        LV_DISPLAY_RENDER_MODE_FULL);

    lv_display_set_flush_cb(lvglDisplay, flushCallback);
    lv_display_set_user_data(lvglDisplay, this);

    if (config->touch && config->touch->supportsLvgl()) {
        if (!config->touch->startLvgl(lvglDisplay)) {
            LOG_W(TAG, "Touch startLvgl failed (non-fatal)");
        }
    }

    refreshSemaphore = xSemaphoreCreateBinary();
    if (!refreshSemaphore) {
        LOG_E(TAG, "Failed to create refresh semaphore");
        lv_display_delete(lvglDisplay);
        lvglDisplay = nullptr;
        for (auto& buf : drawBuf) {
            heap_caps_free(buf);
            buf = nullptr;
        }
        return false;
    }

    if (xTaskCreate(
            epdRefreshTask, "epdRefresh",
            4096, this,
            tskIDLE_PRIORITY + 2,
            &refreshTaskHandle) != pdPASS) {
        LOG_E(TAG, "Failed to create refresh task");
        vSemaphoreDelete(refreshSemaphore);
        refreshSemaphore = nullptr;
        lv_display_delete(lvglDisplay);
        lvglDisplay = nullptr;
        for (auto& buf : drawBuf) {
            heap_caps_free(buf);
            buf = nullptr;
        }
        return false;
    }

    LOG_I(TAG, "LVGL started  %dx%d  I1  FULL  async flush", w, h);
    return true;
}

bool Ssd1685Display::stopLvgl()
{
    if (!lvglDisplay) return true;

    if (config->touch) config->touch->stopLvgl();

    if (refreshTaskHandle) {
        vTaskDelete(refreshTaskHandle);
        refreshTaskHandle = nullptr;
    }
    if (refreshSemaphore) {
        vSemaphoreDelete(refreshSemaphore);
        refreshSemaphore = nullptr;
    }

    lv_display_delete(lvglDisplay);
    lvglDisplay = nullptr;

    for (auto& buf : drawBuf) {
        heap_caps_free(buf);
        buf = nullptr;
    }
    bufSize = 0;

    LOG_I(TAG, "LVGL stopped");
    return true;
}

esp_err_t Ssd1685Display::clearScreen(uint8_t colorByte)
{
    if (!panelHandle) return ESP_ERR_INVALID_STATE;
    return esp_lcd_ssd1685_clear(panelHandle, colorByte);
}

esp_err_t Ssd1685Display::sleep()
{
    if (!panelHandle) return ESP_ERR_INVALID_STATE;
    return esp_lcd_ssd1685_sleep(panelHandle, SSD1685_DEEP_SLEEP_MODE1);
}
