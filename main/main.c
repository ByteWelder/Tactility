#include <esp_err.h>
#include <esp_log.h>
#include <esp_check.h>

#include <nanobake.h>
// Nanobake board support with drivers:
#include <board_2432s024_touch.h>
#include <board_2432s024_display.h>
#include <esp_lvgl_port.h>

static const char *TAG = "main";

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

    app_main_lvgl(&platform);
}
