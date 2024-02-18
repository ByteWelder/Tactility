#include "config.h"

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "lvgl.h"

#define TAG "waveshare_s3_touch_i2c"

static esp_lcd_touch_handle_t touch_init_internal() {
    static esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    static esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_LOGI(TAG, "Initialize touch IO");
    /* Touch IO handle */
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)WAVESHARE_TOUCH_I2C_PORT, &tp_io_config, &tp_io_handle));
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = WAVESHARE_LCD_VER_RES,
        .y_max = WAVESHARE_LCD_HOR_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    /* Initialize touch */
    ESP_LOGI(TAG, "Initialize touch controller GT911");
    esp_lcd_touch_handle_t tp = NULL;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp));

    return tp;
}

static void touch_callback(lv_indev_t* indev, lv_indev_data_t* data) {
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    /* Read touch controller data */
    esp_lcd_touch_handle_t touch_handle = lv_indev_get_user_data(indev);
    esp_lcd_touch_read_data(touch_handle);

    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_handle, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void ws3t_touch_init(lv_display_t* display) {
    esp_lcd_touch_handle_t touch_handle = touch_init_internal();

    ESP_LOGI(TAG, "Register display indev to LVGL");

    static lv_indev_t* device = NULL;
    device = lv_indev_create();
    lv_indev_set_type(device, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(device, &touch_callback);
    lv_indev_set_display(device, display);
    lv_indev_set_user_data(device, touch_handle);
}
