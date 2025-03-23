#include "CST820Touch.h"
#include "CYD2432S022CConstants.h"
#include "esp_log.h"
#include <lvgl.h>
#include <inttypes.h>

static const char *TAG = "CST820Touch";

CST820Touch::CST820Touch(std::unique_ptr<Configuration> config)
    : config_(std::move(config)) {}

bool CST820Touch::start(lv_display_t* display) {
    ESP_LOGI(TAG, "Starting CST820 touch...");
    indev_ = lv_indev_create();
    lv_indev_set_type(indev_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_, [](lv_indev_t* indev, lv_indev_data_t* data) {
        auto* touch = static_cast<CST820Touch*>(lv_indev_get_user_data(indev));
        touch->read_input(data);
    });
    lv_indev_set_user_data(indev_, this);
    lv_indev_set_display(indev_, display);
    display_ = display;  // Store the display pointer for rotation
    return true;
}

bool CST820Touch::stop() {
    ESP_LOGI(TAG, "Stopping CST820 touch...");
    if (indev_) {
        lv_indev_delete(indev_);
        indev_ = nullptr;
    }
    display_ = nullptr;
    return true;
}

bool CST820Touch::read_input(lv_indev_data_t* data) {
    uint8_t touch_data[6];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CYD_2432S022C_TOUCH_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x01, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CYD_2432S022C_TOUCH_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, touch_data, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(config_->i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(err));
        data->state = LV_INDEV_STATE_RELEASED;
        return false;
    }

    uint8_t finger_num = touch_data[1];
    if (finger_num > 0) {
        // Log raw touch data for debugging
        ESP_LOGD(TAG, "Raw touch data: %02x %02x %02x %02x %02x %02x",
                 touch_data[0], touch_data[1], touch_data[2], touch_data[3],
                 touch_data[4], touch_data[5]);

        // Raw hardware coordinates (12-bit values, 0-4095)
        uint16_t raw_x = ((touch_data[2] & 0x0F) << 8) | touch_data[3];
        uint16_t raw_y = ((touch_data[4] & 0x0F) << 8) | touch_data[5];

        // Scale to hardware resolution (240x320)
        uint16_t x = (raw_x * 240) / 4096;
        uint16_t y = (raw_y * 320) / 4096;

        // Transform coordinates based on display rotation
        lv_display_rotation_t rotation = lv_display_get_rotation(display_);
        int32_t logical_x, logical_y;

        ESP_LOGI(TAG, "Scaled touch: x=%" PRIu16 ", y=%" PRIu16 ", rotation=%d", x, y, rotation);

        switch (rotation) {
            case LV_DISPLAY_ROTATION_90:
                // Corrected transformation
                logical_x = x * 319 / 239;        // Map x correctly
                logical_y = y * 239 / 319;        // Map y correctly
                break;
            case LV_DISPLAY_ROTATION_270:
                logical_x = x;
                logical_y = y;
                break;
            case LV_DISPLAY_ROTATION_180:
                logical_x = y;
                logical_y = 240 - x - 1;
                break;
            case LV_DISPLAY_ROTATION_0:
            default:
                logical_x = 320 - y - 1;
                logical_y = x;
                break;
        }

        // Clamp coordinates to logical display bounds
        int32_t max_x = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270) ? 319 : 239;
        int32_t max_y = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270) ? 239 : 319;
        if (logical_x < 0) logical_x = 0;
        if (logical_x > max_x) logical_x = max_x;
        if (logical_y < 0) logical_y = 0;
        if (logical_y > max_y) logical_y = max_y;

        ESP_LOGI(TAG, "Transformed touch: logical x=%" PRId32 ", y=%" PRId32, logical_x, logical_y);

        data->point.x = logical_x;
        data->point.y = logical_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    return false; // No more data to read
}

std::shared_ptr<tt::hal::touch::TouchDevice> createCST820Touch() {
    auto configuration = std::make_unique<CST820Touch::Configuration>(
        CYD_2432S022C_TOUCH_I2C_PORT,
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION
    );

    return std::make_shared<CST820Touch>(std::move(configuration));
}
