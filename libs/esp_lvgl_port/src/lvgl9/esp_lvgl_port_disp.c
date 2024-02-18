/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"

#if (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 4, 4)) || (ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(5, 0, 0))
#define LVGL_PORT_HANDLE_FLUSH_READY 0
#else
#define LVGL_PORT_HANDLE_FLUSH_READY 1
#endif

static const char *TAG = "LVGL";

/*******************************************************************************
* Types definitions
*******************************************************************************/

typedef struct {
    esp_lcd_panel_io_handle_t io_handle;      /* LCD panel IO handle */
    esp_lcd_panel_handle_t    panel_handle;   /* LCD panel handle */
    lvgl_port_rotation_cfg_t  rotation;       /* Default values of the screen rotation */
    lv_color16_t              *draw_buffs[2]; /* Display draw buffers */
    lv_display_t              *disp_drv;      /* LVGL display driver */
    struct {
        unsigned int monochrome: 1;  /* True, if display is monochrome and using 1bit for 1px */
        unsigned int swap_bytes: 1;  /* Swap bytes in RGB656 (16-bit) before send to LCD driver */
    } flags;
} lvgl_port_display_ctx_t;

/*******************************************************************************
* Function definitions
*******************************************************************************/

#if LVGL_PORT_HANDLE_FLUSH_READY
static bool lvgl_port_flush_ready_callback(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
#endif
static void lvgl_port_flush_callback(lv_display_t *drv, const lv_area_t *area, uint8_t *color_map);
static void lvgl_port_disp_size_update_callback(lv_event_t *e);

/*******************************************************************************
* Public API functions
*******************************************************************************/

lv_display_t *lvgl_port_add_disp(const lvgl_port_display_cfg_t *disp_cfg)
{
    esp_err_t ret = ESP_OK;
    lv_display_t *disp = NULL;
    lv_color16_t *buf1 = NULL;
    lv_color16_t *buf2 = NULL;
    assert(disp_cfg != NULL);
    assert(disp_cfg->io_handle != NULL);
    assert(disp_cfg->panel_handle != NULL);
    assert(disp_cfg->buffer_size > 0);
    assert(disp_cfg->hres > 0);
    assert(disp_cfg->vres > 0);

    /* Display context */
    lvgl_port_display_ctx_t *disp_ctx = malloc(sizeof(lvgl_port_display_ctx_t));
    ESP_GOTO_ON_FALSE(disp_ctx, ESP_ERR_NO_MEM, err, TAG, "Not enough memory for display context allocation!");
    disp_ctx->io_handle = disp_cfg->io_handle;
    disp_ctx->panel_handle = disp_cfg->panel_handle;
    disp_ctx->rotation.swap_xy = disp_cfg->rotation.swap_xy;
    disp_ctx->rotation.mirror_x = disp_cfg->rotation.mirror_x;
    disp_ctx->rotation.mirror_y = disp_cfg->rotation.mirror_y;
    disp_ctx->flags.swap_bytes = disp_cfg->flags.swap_bytes;

    uint32_t buff_caps = MALLOC_CAP_DEFAULT;
    if (disp_cfg->flags.buff_dma && disp_cfg->flags.buff_spiram) {
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "Alloc DMA capable buffer in SPIRAM is not supported!");
    } else if (disp_cfg->flags.buff_dma) {
        buff_caps = MALLOC_CAP_DMA;
    } else if (disp_cfg->flags.buff_spiram) {
        buff_caps = MALLOC_CAP_SPIRAM;
    }

    /* alloc draw buffers used by LVGL */
    /* it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized */
    buf1 = heap_caps_malloc(disp_cfg->buffer_size * sizeof(lv_color16_t), buff_caps);
    ESP_GOTO_ON_FALSE(buf1, ESP_ERR_NO_MEM, err, TAG, "Not enough memory for LVGL buffer (buf1) allocation!");
    if (disp_cfg->double_buffer) {
        buf2 = heap_caps_malloc(disp_cfg->buffer_size * sizeof(lv_color16_t), buff_caps);
        ESP_GOTO_ON_FALSE(buf2, ESP_ERR_NO_MEM, err, TAG, "Not enough memory for LVGL buffer (buf2) allocation!");
    }

    disp_ctx->draw_buffs[0] = buf1;
    disp_ctx->draw_buffs[1] = buf2;

    lvgl_port_lock(0);
    disp = lv_display_create(disp_cfg->hres, disp_cfg->vres);

    /* Monochrome display settings */
    if (disp_cfg->monochrome) {
        /* When using monochromatic display, there must be used full bufer! */
        ESP_GOTO_ON_FALSE((disp_cfg->hres * disp_cfg->vres == disp_cfg->buffer_size), ESP_ERR_INVALID_ARG, err, TAG, "Monochromatic display must using full buffer!");

        disp_ctx->flags.monochrome = 1;

        //lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
        lv_display_set_buffers(disp, buf1, buf2, disp_cfg->buffer_size * sizeof(lv_color16_t), LV_DISPLAY_RENDER_MODE_FULL);
    } else {
        lv_display_set_buffers(disp, buf1, buf2, disp_cfg->buffer_size * sizeof(lv_color16_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    }


    lv_display_set_flush_cb(disp, lvgl_port_flush_callback);
    lv_display_add_event_cb(disp, lvgl_port_disp_size_update_callback, LV_EVENT_RESOLUTION_CHANGED, disp_ctx);

    lv_display_set_user_data(disp, disp_ctx);
    disp_ctx->disp_drv = disp;

#if LVGL_PORT_HANDLE_FLUSH_READY
    /* Register done callback */
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = lvgl_port_flush_ready_callback,
    };
    esp_lcd_panel_io_register_event_callbacks(disp_ctx->io_handle, &cbs, disp_ctx->disp_drv);
#endif

    lvgl_port_unlock();

err:
    if (ret != ESP_OK) {
        if (buf1) {
            free(buf1);
        }
        if (buf2) {
            free(buf2);
        }
        if (disp_ctx) {
            free(disp_ctx);
        }
    }

    return disp;
}

esp_err_t lvgl_port_remove_disp(lv_display_t *disp)
{
    assert(disp);
    lvgl_port_display_ctx_t *disp_ctx = (lvgl_port_display_ctx_t *)lv_display_get_user_data(disp);

    lvgl_port_lock(0);
    lv_disp_remove(disp);
    lvgl_port_unlock();

    if (disp_ctx->draw_buffs[0]) {
        free(disp_ctx->draw_buffs[0]);
    }

    if (disp_ctx->draw_buffs[1]) {
        free(disp_ctx->draw_buffs[1]);
    }

    free(disp_ctx);

    return ESP_OK;
}

void lvgl_port_flush_ready(lv_display_t *disp)
{
    assert(disp);
    lv_disp_flush_ready(disp);
}

/*******************************************************************************
* Private functions
*******************************************************************************/

#if LVGL_PORT_HANDLE_FLUSH_READY
static bool lvgl_port_flush_ready_callback(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp_drv = (lv_display_t *)user_ctx;
    assert(disp_drv != NULL);
    lv_disp_flush_ready(disp_drv);
    return false;
}
#endif

static void _lvgl_port_transform_monochrome(lv_display_t *display, const lv_area_t *area, uint8_t *color_map)
{
    uint8_t *buf = color_map;
    lv_color16_t *color = (lv_color16_t *)color_map;
    uint16_t hor_res = lv_display_get_physical_horizontal_resolution(display);
    uint16_t ver_res = lv_display_get_physical_vertical_resolution(display);
    uint16_t res = hor_res;
    bool swap_xy = (lv_display_get_rotation(display) == LV_DISPLAY_ROTATION_90 || lv_display_get_rotation(display) == LV_DISPLAY_ROTATION_270);

    int x1 = area->x1;
    int x2 = area->x2;
    int y1 = area->y1;
    int y2 = area->y2;

    int out_x, out_y;
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            bool chroma_color = (color[hor_res * y + x].blue > 16);

            if (swap_xy) {
                out_x = y;
                out_y = x;
                res = ver_res;
            } else {
                out_x = x;
                out_y = y;
                res = hor_res;
            }

            /* Write to the buffer as required for the display.
            * It writes only 1-bit for monochrome displays mapped vertically.*/
            buf = color_map + res * (out_y >> 3) + (out_x);
            if (chroma_color) {
                (*buf) &= ~(1 << (out_y % 8));
            } else {
                (*buf) |= (1 << (out_y % 8));
            }
        }
    }
}

static void lvgl_port_flush_callback(lv_display_t *drv, const lv_area_t *area, uint8_t *color_map)
{
    assert(drv != NULL);
    lvgl_port_display_ctx_t *disp_ctx = (lvgl_port_display_ctx_t *)lv_display_get_user_data(drv);
    assert(disp_ctx != NULL);

    //TODO: try to use SPI_SWAP_DATA_RX from https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32s3/api-reference/peripherals/spi_master.html#c.SPI_SWAP_DATA_TX
    if (disp_ctx->flags.swap_bytes) {
        size_t len = lv_area_get_size(area);
        lv_draw_sw_rgb565_swap(color_map, len);
    }

    /* Transfor data in buffer for monochromatic screen */
    if (disp_ctx->flags.monochrome) {
        _lvgl_port_transform_monochrome(drv, area, color_map);
    }

    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(disp_ctx->panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void lvgl_port_disp_size_update_callback(lv_event_t *e)
{
    assert(e);
    lvgl_port_display_ctx_t *disp_ctx = (lvgl_port_display_ctx_t *)e->user_data;
    assert(disp_ctx != NULL);
    esp_lcd_panel_handle_t panel_handle = disp_ctx->panel_handle;

    /* Solve rotation screen and touch */
    switch (lv_display_get_rotation(disp_ctx->disp_drv)) {
    case LV_DISPLAY_ROTATION_0:
        /* Rotate LCD display */
        esp_lcd_panel_swap_xy(panel_handle, disp_ctx->rotation.swap_xy);
        esp_lcd_panel_mirror(panel_handle, disp_ctx->rotation.mirror_x, disp_ctx->rotation.mirror_y);
        break;
    case LV_DISPLAY_ROTATION_90:
        /* Rotate LCD display */
        esp_lcd_panel_swap_xy(panel_handle, !disp_ctx->rotation.swap_xy);
        if (disp_ctx->rotation.swap_xy) {
            esp_lcd_panel_mirror(panel_handle, !disp_ctx->rotation.mirror_x, disp_ctx->rotation.mirror_y);
        } else {
            esp_lcd_panel_mirror(panel_handle, disp_ctx->rotation.mirror_x, !disp_ctx->rotation.mirror_y);
        }
        break;
    case LV_DISPLAY_ROTATION_180:
        /* Rotate LCD display */
        esp_lcd_panel_swap_xy(panel_handle, disp_ctx->rotation.swap_xy);
        esp_lcd_panel_mirror(panel_handle, !disp_ctx->rotation.mirror_x, !disp_ctx->rotation.mirror_y);
        break;
    case LV_DISPLAY_ROTATION_270:
        /* Rotate LCD display */
        esp_lcd_panel_swap_xy(panel_handle, !disp_ctx->rotation.swap_xy);
        if (disp_ctx->rotation.swap_xy) {
            esp_lcd_panel_mirror(panel_handle, disp_ctx->rotation.mirror_x, !disp_ctx->rotation.mirror_y);
        } else {
            esp_lcd_panel_mirror(panel_handle, !disp_ctx->rotation.mirror_x, disp_ctx->rotation.mirror_y);
        }
        break;
    }
}
