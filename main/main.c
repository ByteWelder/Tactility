#include <esp_err.h>
#include <esp_log.h>
#include <esp_check.h>
#include <esp_lcd_panel_io.h>
#include <esp_lvgl_port.h>

#include <nanobake.h>
// Nanobake board support with drivers:
#include <board_2432s024_touch.h>
#include <board_2432s024_display.h>

#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (50)

static const char *TAG = "main";


// LVGL
static lv_disp_t* lvgl_disp = NULL;
static lv_indev_t* lvgl_touch_indev = NULL;


static esp_err_t app_lvgl_init(
    nb_platform_t* platform
) {
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl port init failed");

    // Add display
    ESP_LOGD(TAG, "lvgl add display");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = platform->display.io_handle,
        .panel_handle = platform->display.display_handle,
        .buffer_size = platform->display.horizontal_resolution * platform->display.draw_buffer_height * (platform->display.bits_per_pixel / 8),
        .double_buffer = EXAMPLE_LCD_DRAW_BUFF_DOUBLE,
        .hres = platform->display.horizontal_resolution,
        .vres = platform->display.vertical_resolution,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    // Add touch
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = platform->touch.touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);
    ESP_RETURN_ON_FALSE(lvgl_touch_indev != NULL, ESP_FAIL, TAG, "failed to add touch to lvgl");

    return ESP_OK;
}

static void app_button_cb(lv_event_t* e) {
    ESP_LOGI(TAG, "tap");
}

static void app_main_lvgl(nb_platform_t* platform) {
    lv_obj_t *scr = lv_scr_act();

    lvgl_port_lock(0);

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_recolor(label, true);
    lv_obj_set_width(label, 240);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label, "Hello, world!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *btn = lv_btn_create(scr);
    label = lv_label_create(btn);
    lv_label_set_text_static(label, "Button");
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_add_event_cb(btn, app_button_cb, LV_EVENT_CLICKED, NULL);

    lvgl_port_unlock();
}

void app_main(void) {
    nb_platform_config_t platform_config = {
        .touch_driver = board_2432s024_create_touch_driver(),
        .display_driver = board_2432s024_create_display_driver()
    };

    static nb_platform_t platform;
    ESP_ERROR_CHECK(nb_platform_create(platform_config, &platform));

    ESP_ERROR_CHECK(app_lvgl_init(&platform));

    app_main_lvgl(&platform);
}
