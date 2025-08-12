#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#if CONFIG_LCD_ENABLE_DEBUG_LOG
// The local log level must be defined before including esp_log.h
// Set the maximum log level for this source file
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif
#include "esp_log.h"
#include "esp_check.h"
#include "esp_attr.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_gdeq.h"

#include "TdeckConstants.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_gdeq_commands.h"

static const char* TAG = "lcd_panel.epaper";

typedef struct {
    esp_lcd_epaper_panel_cb_t callback_ptr;
    void* args;
} epaper_panel_callback_t;

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    // --- Normal configurations
    // Configurations from epaper_ssd1681_conf
    bool full_refresh;
    bool fast_refresh;
    int busy_gpio_num;
    // Configurations from e-Paper specific public functions
    epaper_panel_callback_t epaper_refresh_done_isr_callback;
    uint8_t* _framebuffer;
} epaper_panel_t;

// --- Utility functions
static esp_err_t process_bitmap(esp_lcd_panel_t* panel, int buffer_size, const void* color_data);
static esp_err_t panel_epaper_wait_busy(esp_lcd_panel_t* panel);
// --- Used to implement esp_lcd_panel_interface
static esp_err_t epaper_panel_del(esp_lcd_panel_t* panel);
static esp_err_t epaper_panel_reset(esp_lcd_panel_t* panel);
static esp_err_t epaper_panel_init(esp_lcd_panel_t* panel);
static esp_err_t epaper_panel_draw_bitmap(esp_lcd_panel_t* panel, int x_start, int y_start, int x_end, int y_end, const void* color_data);
static esp_err_t epaper_panel_invert_color(esp_lcd_panel_t* panel, bool invert_color_data);
static esp_err_t epaper_panel_mirror(esp_lcd_panel_t* panel, bool mirror_x, bool mirror_y);
static esp_err_t epaper_panel_swap_xy(esp_lcd_panel_t* panel, bool swap_axes);
static esp_err_t epaper_panel_set_gap(esp_lcd_panel_t* panel, int x_gap, int y_gap);
static esp_err_t epaper_panel_disp_on_off(esp_lcd_panel_t* panel, bool on_off);

static esp_err_t panel_epaper_wait_busy(esp_lcd_panel_t* panel) {
    epaper_panel_t* epaper_panel = __containerof(panel, epaper_panel_t, base);
    // Wait until busy pin is high (busy means low)
    while (gpio_get_level(epaper_panel->busy_gpio_num) != 0) { vTaskDelay(pdMS_TO_TICKS(15)); }
    return ESP_OK;
}

esp_err_t epaper_panel_register_event_callbacks(esp_lcd_panel_t* panel, epaper_panel_callbacks_t* cbs, void* user_ctx) {
    ESP_RETURN_ON_FALSE(panel, ESP_ERR_INVALID_ARG, TAG, "panel handler is NULL");
    ESP_RETURN_ON_FALSE(cbs, ESP_ERR_INVALID_ARG, TAG, "cbs is NULL");
    epaper_panel_t* epaper_panel = __containerof(panel, epaper_panel_t, base);
    (epaper_panel->epaper_refresh_done_isr_callback).callback_ptr = cbs->on_epaper_refresh_done;
    (epaper_panel->epaper_refresh_done_isr_callback).args = user_ctx;
    return ESP_OK;
}

static esp_err_t epaper_set_area(esp_lcd_panel_io_handle_t io, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y) {
    uint32_t x = start_x;
    uint32_t y = start_y;
    uint32_t w = end_x - start_x;
    uint32_t h = end_y - start_y;

    uint16_t xe = (x + w - 1) | 0x0007; // byte boundary inclusive (last byte)
    uint16_t ye = y + h - 1;
    x &= 0xFFF8; // byte boundary

    uint8_t data[7] = {x, xe, y / 256, y % 256, ye / 256, ye % 256, 0x01};
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x90, data, sizeof(data)), TAG, "set_area err");

    return ESP_OK;
}

esp_err_t epaper_panel_refresh_screen(esp_lcd_panel_t* panel, bool fast, bool full) {
    ESP_RETURN_ON_FALSE(panel, ESP_ERR_INVALID_ARG, TAG, "panel handler is NULL");
    epaper_panel_t* epaper_panel = __containerof(panel, epaper_panel_t, base);

    if (fast) {
        const int ccset_cmd = 0xe0;
        const uint8_t tsfix_data[1] = {0x02};
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(epaper_panel->io, ccset_cmd, tsfix_data, sizeof(tsfix_data)), TAG, "cascade setting err");

        const int ttset_cmd = 0xe5;
        uint8_t ttset_data[1];

        if (full) {
            ttset_data[0] = 0x5a;
        } else {
            ttset_data[0] = 0x79; // 121
        }

        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(epaper_panel->io, ttset_cmd, ttset_data, sizeof(ttset_data)), TAG, "temperature setting err");
    }

    const int refresh_cmd = 0x50;
    uint8_t refresh_data[1];
    if (full) {
        refresh_data[0] = 0x97;
    } else {
        refresh_data[0] = 0xd7;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(epaper_panel->io, refresh_cmd, refresh_data, sizeof(refresh_data)), TAG, "refresh err");

    // TODO: Remove?
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(epaper_panel->io, GDEQ_CMD_POWER_ON, NULL, 0), TAG, "power on err");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(epaper_panel->io, 0x12, NULL, 0), TAG, "refresh(2) err");

    return ESP_OK;

}

esp_err_t esp_lcd_new_panel_gdeq031t10(
    const esp_lcd_panel_io_handle_t io,
    const esp_lcd_panel_dev_config_t* const panel_dev_config,
    esp_lcd_panel_handle_t* const ret_panel
) {
#if CONFIG_LCD_ENABLE_DEBUG_LOG
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
    ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, TAG, "1 or more args is NULL");
    esp_err_t ret = ESP_OK;
    // --- Allocate epaper_panel memory on HEAP
    epaper_panel_t* epaper_panel = malloc(sizeof(epaper_panel_t));
    ESP_GOTO_ON_FALSE(epaper_panel, ESP_ERR_NO_MEM, err, TAG, "no mem for epaper panel");

    // --- Construct panel & implement interface
    // defaults
    epaper_panel->_framebuffer = NULL;
    epaper_panel->full_refresh = true;
    // configurations
    epaper_panel->io = io;
    // functions
    epaper_panel->base.del = epaper_panel_del;
    epaper_panel->base.reset = epaper_panel_reset;
    epaper_panel->base.init = epaper_panel_init;
    epaper_panel->base.draw_bitmap = epaper_panel_draw_bitmap;
    epaper_panel->base.invert_color = epaper_panel_invert_color;
    epaper_panel->base.set_gap = epaper_panel_set_gap;
    epaper_panel->base.mirror = epaper_panel_mirror;
    epaper_panel->base.swap_xy = epaper_panel_swap_xy;
    epaper_panel->base.disp_on_off = epaper_panel_disp_on_off;
    *ret_panel = &(epaper_panel->base);
    // --- Init framebuffer
    int buffer_size = 240 * 320 / 8;
    epaper_panel->_framebuffer = heap_caps_malloc(buffer_size, MALLOC_CAP_DMA);
    memset(epaper_panel->_framebuffer, 0xff, buffer_size);
    ESP_RETURN_ON_FALSE(epaper_panel->_framebuffer, ESP_ERR_NO_MEM, TAG, "epaper_panel_draw_bitmap allocating buffer memory err");
    // --- Init GPIO
    // init BUSY GPIO
    ESP_LOGD(TAG, "new epaper panel @%p", epaper_panel);
    return ret;
err:
    if (epaper_panel) {
        if (panel_dev_config->reset_gpio_num >= 0) { gpio_reset_pin(panel_dev_config->reset_gpio_num); }
        free(epaper_panel);
    }
    return ret;
}

static esp_err_t epaper_panel_del(esp_lcd_panel_t* panel) {
    epaper_panel_t* epaper_panel = __containerof(panel, epaper_panel_t, base);
    // --- Free allocated RAM
    if (epaper_panel->_framebuffer) {
        // Should not free if buffer is not allocated by driver
        free(epaper_panel->_framebuffer);
    }
    ESP_LOGD(TAG, "del ssd1681 epaper panel @%p", epaper_panel);
    free(epaper_panel);
    return ESP_OK;
}

static esp_err_t epaper_panel_reset(esp_lcd_panel_t* panel) {
    epaper_panel_t* epaper_panel = __containerof(panel, epaper_panel_t, base);
    esp_lcd_panel_io_handle_t io = epaper_panel->io;

    const uint8_t soft_reset_data[2] = {0x1e, 0x0d};
    const int panel_setting_cmd = 0x00;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, panel_setting_cmd, soft_reset_data, sizeof(soft_reset_data)), TAG, "io tx param failed");

    // TODO: Remove?
    vTaskDelay(pdMS_TO_TICKS(1));

    panel_epaper_wait_busy(panel);
    return ESP_OK;
}

esp_err_t epaper_panel_set_bitmap_color(esp_lcd_panel_t* panel, esp_lcd_gdeq_bitmap_color_t color) {
    // Not implemented
    return ESP_FAIL;
}

static esp_err_t epaper_panel_init(esp_lcd_panel_t* panel) {
    epaper_panel_t* epaper_panel = __containerof(panel, epaper_panel_t, base);
    esp_lcd_panel_io_handle_t io = epaper_panel->io;

    epaper_panel->busy_gpio_num = BOARD_EPD_BUSY; // TODO: make it properly configurable
    gpio_set_direction(epaper_panel->busy_gpio_num, GPIO_MODE_INPUT);

    const uint8_t soft_reset_data[2] = {0x1e, 0x0d};
    const int panel_setting_cmd = 0x00;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, panel_setting_cmd, soft_reset_data, sizeof(soft_reset_data)), TAG, "io tx param failed");

    vTaskDelay(pdMS_TO_TICKS(1));

    const uint8_t bwotp_data[2] = {0x1f, 0x0d}; // KW: 3f, KWR: 2F, BWROTP: 0f, BWOTP: 1f
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, panel_setting_cmd, bwotp_data, sizeof(bwotp_data)), TAG, "io tx param failed");

    panel_epaper_wait_busy(panel);

    int buffer_size = 240 * 320 / 8;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(epaper_panel->io, 0x10, epaper_panel->_framebuffer, buffer_size), TAG, "tx buffer 0x10 failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(epaper_panel->io, 0x13, epaper_panel->_framebuffer, buffer_size), TAG, "tx buffer 0x13 failed");

    epaper_panel_refresh_screen(panel, false, true);

    return ESP_OK;
}

static esp_err_t epaper_panel_draw_bitmap(esp_lcd_panel_t* panel, int x_start, int y_start, int x_end, int y_end, const void* color_data) {
    epaper_panel_t* epaper_panel = __containerof(panel, epaper_panel_t, base);
    // --- Assert & check configuration
    ESP_RETURN_ON_FALSE(color_data, ESP_ERR_INVALID_ARG, TAG, "bitmap is null");
    ESP_RETURN_ON_FALSE((x_start < x_end) && (y_start < y_end), ESP_ERR_INVALID_ARG, TAG, "start position must be smaller than end position");
    // --- Calculate coordinates & sizes
    int len_x = abs(x_start - x_end);
    int len_y = abs(y_start - y_end);
    int buffer_size = len_x * len_y / 8;
    // --- Data copy & preprocess
    // prepare buffer
    process_bitmap(panel, buffer_size, color_data);

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(epaper_panel->io, 0x91, NULL, 0), TAG, "tx partial in failed");
    epaper_set_area(epaper_panel->io, x_start, y_start, x_end, y_end);
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(epaper_panel->io, 0x13, color_data, buffer_size), TAG, "tx color failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(epaper_panel->io, 0x92, NULL, 0), TAG, "tx partial in failed");

    epaper_panel_refresh_screen(panel, false, false);

    return ESP_OK;
}

static esp_err_t epaper_panel_invert_color(esp_lcd_panel_t* panel, bool invert_color_data) {
    // Not implemented
    return ESP_FAIL;
}

static esp_err_t epaper_panel_mirror(esp_lcd_panel_t* panel, bool mirror_x, bool mirror_y) {
    // Not implemented
    return ESP_FAIL;
}

static esp_err_t epaper_panel_swap_xy(esp_lcd_panel_t* panel, bool swap_axes) {
    // Not implemented
    return ESP_FAIL;
}

static esp_err_t epaper_panel_set_gap(esp_lcd_panel_t* panel, int x_gap, int y_gap) {
    // Not implemented
    return ESP_FAIL;
}

static esp_err_t epaper_panel_disp_on_off(esp_lcd_panel_t* panel, bool on_off) {
    epaper_panel_t* epaper_panel = __containerof(panel, epaper_panel_t, base);
    if (on_off) { ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(epaper_panel->io, GDEQ_CMD_POWER_ON, NULL, 0), TAG, "power on err"); } else { ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(epaper_panel->io, GDEQ_CMD_POWER_OFF, NULL, 0), TAG, "power off err"); }
    panel_epaper_wait_busy(panel);

    return ESP_OK;
}

static esp_err_t process_bitmap(esp_lcd_panel_t* panel, int buffer_size, const void* color_data) {
    epaper_panel_t* epaper_panel = __containerof(panel, epaper_panel_t, base);
    uint8_t* byte_pixels = (uint8_t*)color_data;
    for (int i = 0; i < buffer_size; i += 8) {
        // Convert byte-sized pixels to bit-sized ones
        uint8_t bit_pixel_group =
            (0b00000001 & byte_pixels[i]) |
            (0b00000010 & byte_pixels[i + 2]) |
            (0b00000100 & byte_pixels[i + 3]) |
            (0b00001000 & byte_pixels[i + 4]) |
            (0b00010000 & byte_pixels[i + 5]) |
            (0b00100000 & byte_pixels[i + 6]) |
            (0b01000000 & byte_pixels[i + 7]) |
            (0b10000000 & byte_pixels[i + 8]);
        epaper_panel->_framebuffer[i] = bit_pixel_group;
    }
    return ESP_OK;
}