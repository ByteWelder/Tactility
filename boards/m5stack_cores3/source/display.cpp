#include "config.h"
#include "tactility_core.h"

#include "esp_err.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io_interface.h"

#include "M5Unified.hpp"

#define TAG "cores3"

#ifdef __cplusplus
extern "C" {
#endif

int shared_x_gap = 0;
int shared_y_gap = 0;

esp_err_t panel_reset(esp_lcd_panel_t* panel) {
    shared_x_gap = 0;
    shared_y_gap = 0;
    return ESP_OK;
}

esp_err_t panel_init(esp_lcd_panel_t *panel) {
    return ESP_OK;
}

esp_err_t panel_del(esp_lcd_panel_t *panel) {
    shared_x_gap = 0;
    shared_y_gap = 0;
    return ESP_OK;
}

esp_err_t panel_draw_bitmap(esp_lcd_panel_t* panel, int x_start, int y_start, int x_end, int y_end, const void* color_data) {
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");

    lgfx::pixelcopy_t pixelcopy;
    pixelcopy.no_convert = true;
    pixelcopy.src_data = color_data;
    pixelcopy.src_width = x_end - x_start + shared_x_gap;
    pixelcopy.src_height = y_end - y_start + shared_y_gap;
    pixelcopy.src_depth = lgfx::rgb565_nonswapped;
    pixelcopy.dst_depth = lgfx::rgb565_nonswapped;

    M5.Display.startWrite();
    M5.Display.pushImage(x_start, y_start, x_end - x_start, y_end - y_start, &pixelcopy, false);
    M5.Display.endWrite();

    // TODO: Remove hack that unblocks wait_for_flushing() in lv_refr.c
    lv_disp_t* display = lv_display_get_default();
    lv_display_flush_ready(display);

    return ESP_OK;
}

esp_err_t panel_mirror(esp_lcd_panel_t *panel, bool x_axis, bool y_axis) {
    TT_LOG_I("TEST", "mirror");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t panel_swap_xy(esp_lcd_panel_t *panel, bool swap_axes) {
    TT_LOG_I("TEST", "swap_xy");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t panel_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap) {
    TT_LOG_I("TEST", "set_gap %d %d", x_gap, y_gap);
    shared_x_gap = x_gap;
    shared_y_gap = y_gap;
    return ESP_OK;
}

esp_err_t panel_invert_color(esp_lcd_panel_t *panel, bool invert_color_data) {
    TT_LOG_I("TEST", "panel_invert_color");
    auto* device = (lgfx::Panel_Device*)panel->user_data;
    device->setInvert(invert_color_data);
    return ESP_OK;
}

esp_err_t panel_disp_on_off(esp_lcd_panel_t *panel, bool on_off) {
    TT_LOG_I("TEST", "panel_disp_on_off");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t panel_disp_sleep(esp_lcd_panel_t *panel, bool sleep) {
    TT_LOG_I("TEST", "panel_disp_sleep");
    auto* device = (lgfx::Panel_Device*)panel->user_data;
    device->setSleep(sleep);
    return ESP_OK;
}


// Region panel io placeholders

esp_err_t io_rx_param(esp_lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size) {
    return ESP_OK;
}

esp_err_t io_tx_param(esp_lcd_panel_io_t *io, int lcd_cmd, const void *param, size_t param_size){
    return ESP_OK;
}

esp_err_t io_tx_color(esp_lcd_panel_io_t *io, int lcd_cmd, const void *color, size_t color_size){
    return ESP_OK;
}

esp_err_t io_del(esp_lcd_panel_io_t *io) {
    return ESP_OK;
}

esp_err_t io_register_event_callbacks(esp_lcd_panel_io_t *io, const esp_lcd_panel_io_callbacks_t *cbs, void *user_ctx) {
    return ESP_OK;
}

// endregion

lv_disp_t* cores3_display_init() {
    TT_LOG_I(TAG, "Display init");

    M5.Display.begin();

    esp_lcd_panel_io_handle_t io_handle = (esp_lcd_panel_io_handle_t)malloc(sizeof(esp_lcd_panel_io_t));
    io_handle->rx_param = io_rx_param;
    io_handle->tx_param = io_tx_param;
    io_handle->tx_color = io_tx_color;
    io_handle->del = io_del;
    io_handle->register_event_callbacks = io_register_event_callbacks;

    auto panel_handle = (esp_lcd_panel_handle_t)malloc(sizeof(esp_lcd_panel_t));
    *panel_handle = {
        .reset = panel_reset,
        .init = panel_init,
        .del = panel_del,
        .draw_bitmap = panel_draw_bitmap,
        .mirror = panel_mirror,
        .swap_xy = panel_swap_xy,
        .set_gap = panel_set_gap,
        .invert_color = panel_invert_color,
        .disp_on_off = panel_disp_on_off,
        .disp_sleep = panel_disp_sleep,
        .user_data = M5.Display.panel()
    };

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = CORES3_LCD_DRAW_BUFFER_SIZE,
        .double_buffer = false,
        .trans_size = CORES3_SPI2_TRANSACTION_LIMIT,
        .hres = CORES3_LCD_HORIZONTAL_RESOLUTION,
        .vres = CORES3_LCD_VERTICAL_RESOLUTION,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
            .sw_rotate = false,
            .swap_bytes = false
        }
    };

    lv_display_t* display = lvgl_port_add_disp(&disp_cfg);

    return display;
}

#ifdef __cplusplus
}
#endif
