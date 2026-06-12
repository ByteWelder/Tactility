#include "Ssd1685Display.h"

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_check.h>
#include <tactility/log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

static constexpr const char* TAG = "Ssd1685Display";

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

Ssd1685Display::Ssd1685Display(std::unique_ptr<Configuration> cfg)
    : config(std::move(cfg))
{}

Ssd1685Display::~Ssd1685Display()
{
    if (lvglDisplay) stopLvgl();
    if (started)     stop();
}

// ---------------------------------------------------------------------------
// EPD refresh task — runs outside the LVGL lock so the slow EPD refresh
// doesn't block LVGL rendering.
// ---------------------------------------------------------------------------

void Ssd1685Display::epdRefreshTask(void* params)
{
    auto* self = static_cast<Ssd1685Display*>(params);
    while (true) {
        if (xSemaphoreTake(self->refreshSemaphore, portMAX_DELAY) == pdTRUE) {
            esp_lcd_ssd1685_refresh(self->panelHandle, self->config->refreshMode);
        }
    }
}

// ---------------------------------------------------------------------------
// LVGL rotation helper
// ---------------------------------------------------------------------------

lv_display_rotation_t Ssd1685Display::lvglRotation(uint8_t rotation)
{
    switch (rotation) {
    case 1:  return LV_DISPLAY_ROTATION_90;
    case 2:  return LV_DISPLAY_ROTATION_180;
    case 3:  return LV_DISPLAY_ROTATION_270;
    default: return LV_DISPLAY_ROTATION_0;
    }
}

// ---------------------------------------------------------------------------
// flushCallback
//
// LVGL calls this with L8 pixel data (1 byte per pixel, 0=black, 255=white)
// in PHYSICAL PORTRAIT coordinates.  LVGL already performed any sw rotation
// before reaching here, so we always receive portrait-oriented data regardless
// of the logical orientation the application thinks it has.
//
// Steps:
//   1. For each pixel: threshold L8 -> 1 bit (>=128 = white/1, <128 = black/0)
//   2. Pack 8 pixels per byte, MSB first (SSD1685 convention)
//   3. Call draw_bitmap in portrait mode — no swap_xy, no software rotation
//   4. On last flush of the frame, signal the refresh task
// ---------------------------------------------------------------------------

void Ssd1685Display::flushCallback(lv_display_t* disp,
                                    const lv_area_t* area,
                                    uint8_t* pixelMap)
{
    auto* self = static_cast<Ssd1685Display*>(lv_display_get_user_data(disp));
    if (!self || !self->panelHandle || !pixelMap || !self->packedBuf) {
        lv_display_flush_ready(disp);
        return;
    }

    const int x1 = area->x1;
    const int y1 = area->y1;
    const int x2 = area->x2;
    const int y2 = area->y2;
    const int w  = x2 - x1 + 1;
    const int h  = y2 - y1 + 1;

    /* L8 source stride = physical display width (LVGL always provides a full-width
     * row when RENDER_MODE_FULL is active, even for sub-areas). */
    const int src_stride = (int)lv_display_get_horizontal_resolution(disp);

    /* 1bpp packed output stride: one bit per source pixel, MSB first. */
    const int dst_stride = (w + 7) / 8;

    for (int row = 0; row < h; row++) {
        const uint8_t* src = pixelMap + (size_t)row * src_stride + x1;
        uint8_t*       dst = self->packedBuf + (size_t)row * dst_stride;

        for (int col = 0; col < w; col++) {
            /* SSD1685: 1 = white, 0 = black — same polarity as L8 >=128 = white */
            uint8_t bit = (src[col] >= 128) ? 1u : 0u;
            int byte_idx = col >> 3;
            int bit_pos  = 7 - (col & 7);  /* MSB first */
            if (bit) {
                dst[byte_idx] |=  (uint8_t)(1u << bit_pos);
            } else {
                dst[byte_idx] &= (uint8_t)~(1u << bit_pos);
            }
        }
    }

    /* draw_bitmap uses the non-swap_xy path: hardware is always portrait.
     * gapX is applied inside draw_bitmap (non-swap path adds it before set_ram_window). */
    esp_lcd_panel_draw_bitmap(
        self->panelHandle,
        x1, y1, x2 + 1, y2 + 1,
        self->packedBuf);

    if (lv_display_flush_is_last(disp)) {
        xSemaphoreGive(self->refreshSemaphore);
    }

    lv_display_flush_ready(disp);
}

// ---------------------------------------------------------------------------
// start / stop
// ---------------------------------------------------------------------------

bool Ssd1685Display::start()
{
    if (started) {
        LOG_W(TAG, "Already started");
        return true;
    }

    /* SPI panel IO */
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

    /* SSD1685 panel */
    esp_lcd_panel_ssd1685_config_t epd_cfg = {
        .busy_gpio_num        = config->busyPin,
        .busy_timeout_ms      = config->busyTimeoutMs,
        .panel_width          = config->width,
        .panel_height         = config->height,
        .non_copy_mode        = true,   /* we call refresh manually */
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

    /* Apply source-line gap offset.
     * The hardware always runs in portrait mode (no swap_xy / mirror).
     * gapX shifts the RAM window right by gapX sources so pixel (0,0) in
     * the application maps to the first live source (S8 on GDEY029T71H). */
    if (config->gapX != 0 || config->gapY != 0) {
        esp_lcd_panel_set_gap(panelHandle, config->gapX, config->gapY);
    }

    /* No hardware rotation: LVGL handles rotation in software. */

    LOG_I(TAG, "Initial clear...");
    esp_lcd_ssd1685_clear(panelHandle, 0xFF);

    started = true;
    LOG_I(TAG, "Started  physical=%dx%d  rotation=%d (LVGL sw)  gap=(%d,%d)",
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

// ---------------------------------------------------------------------------
// startLvgl / stopLvgl
// ---------------------------------------------------------------------------

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

    const uint16_t pw = physWidth();   /* 168 */
    const uint16_t ph = physHeight();  /* 384 */

    /* L8 draw buffer: one byte per physical pixel.
     * LVGL renders the logical (rotated) scene into this buffer in portrait
     * physical layout before calling our flush callback. */
    size_t draw_size = (size_t)pw * ph;  /* 168 * 384 = 64512 bytes */

    drawBuf = static_cast<uint8_t*>(
        heap_caps_malloc(draw_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!drawBuf) {
        drawBuf = static_cast<uint8_t*>(
            heap_caps_malloc(draw_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    }
    if (!drawBuf) {
        LOG_E(TAG, "Failed to allocate L8 draw buffer (%zu bytes)", draw_size);
        return false;
    }
    memset(drawBuf, 0xFF, draw_size);  /* initialise to white */

    /* 1bpp packed buffer: used by flushCallback to stage the thresholded data.
     * Width is the physical portrait width (rows sent left-to-right without gap
     * in caller's view; draw_bitmap adds gapX before setting the RAM window).
     * Size = ceil(pw / 8) * ph */
    size_t packed_stride = ((size_t)pw + 7u) / 8u;  /* 21 bytes/row */
    size_t packed_size   = packed_stride * ph;        /* 21 * 384 = 8064 bytes */

    packedBuf = static_cast<uint8_t*>(
        heap_caps_malloc(packed_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!packedBuf) {
        packedBuf = static_cast<uint8_t*>(
            heap_caps_malloc(packed_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    }
    if (!packedBuf) {
        LOG_E(TAG, "Failed to allocate 1bpp packed buffer (%zu bytes)", packed_size);
        heap_caps_free(drawBuf); drawBuf = nullptr;
        return false;
    }
    memset(packedBuf, 0xFF, packed_size);

    /* Create the LVGL display at physical portrait dimensions.
     * LVGL will sw-rotate the logical canvas to match the requested rotation. */
    lvglDisplay = lv_display_create(pw, ph);
    if (!lvglDisplay) {
        LOG_E(TAG, "lv_display_create failed");
        heap_caps_free(packedBuf); packedBuf = nullptr;
        heap_caps_free(drawBuf);   drawBuf   = nullptr;
        return false;
    }

    lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_L8);

    /* Ask LVGL to rotate the logical canvas.  With RENDER_MODE_FULL LVGL
     * renders the full screen each frame, rotating in software into drawBuf
     * before calling our flush callback with portrait-physical data. */
    lv_display_set_rotation(lvglDisplay, lvglRotation(config->rotation));

    lv_display_set_buffers(
        lvglDisplay,
        drawBuf, nullptr,
        draw_size,
        LV_DISPLAY_RENDER_MODE_FULL);

    lv_display_set_flush_cb(lvglDisplay, flushCallback);
    lv_display_set_user_data(lvglDisplay, this);

    if (config->touch && config->touch->supportsLvgl()) {
        if (!config->touch->startLvgl(lvglDisplay)) {
            LOG_W(TAG, "Touch startLvgl failed (non-fatal)");
        }
    }

    /* Semaphore + task for async EPD refresh (keeps LVGL unblocked). */
    refreshSemaphore = xSemaphoreCreateBinary();
    if (!refreshSemaphore) {
        LOG_E(TAG, "Failed to create refresh semaphore");
        lv_display_delete(lvglDisplay); lvglDisplay = nullptr;
        heap_caps_free(packedBuf); packedBuf = nullptr;
        heap_caps_free(drawBuf);   drawBuf   = nullptr;
        return false;
    }

    if (xTaskCreate(
            epdRefreshTask, "epdRefresh",
            4096, this,
            tskIDLE_PRIORITY + 2,
            &refreshTaskHandle) != pdPASS) {
        LOG_E(TAG, "Failed to create refresh task");
        vSemaphoreDelete(refreshSemaphore); refreshSemaphore = nullptr;
        lv_display_delete(lvglDisplay); lvglDisplay = nullptr;
        heap_caps_free(packedBuf); packedBuf = nullptr;
        heap_caps_free(drawBuf);   drawBuf   = nullptr;
        return false;
    }

    /* Derive the logical (post-rotation) canvas size for the log message. */
    bool is_landscape = (config->rotation == 1 || config->rotation == 3);
    uint16_t logical_w = is_landscape ? ph : pw;
    uint16_t logical_h = is_landscape ? pw : ph;

    LOG_I(TAG, "LVGL started  physical=%dx%d  logical=%dx%d  L8  FULL  async refresh",
          pw, ph, logical_w, logical_h);
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

    if (packedBuf) { heap_caps_free(packedBuf); packedBuf = nullptr; }
    if (drawBuf)   { heap_caps_free(drawBuf);   drawBuf   = nullptr; }

    LOG_I(TAG, "LVGL stopped");
    return true;
}

// ---------------------------------------------------------------------------
// Convenience wrappers
// ---------------------------------------------------------------------------

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
