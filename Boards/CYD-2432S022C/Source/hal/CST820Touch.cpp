#include "CST820Touch.h"
#include "CYD2432S022CConstants.h"
#include "touch_calibration_data.h"
#include "esp_log.h"
#include <lvgl.h>
#include <inttypes.h>
#include <esp_timer.h>
#include <nvs_flash.h>

static const char *TAG = "CST820Touch";

CST820Touch::CST820Touch(std::unique_ptr<Configuration> config)
    : config_(std::move(config)) {}

bool CST820Touch::start(lv_display_t* display) {
    ESP_LOGI(TAG, "Starting CST820 touch...");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs to be erased");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(err));
    } else {
        nvs_handle_t handle;
        err = nvs_open("touch_calib", NVS_READONLY, &handle);
        if (err == ESP_OK) {
            for (int i = 0; i < 4; i++) {
                char key[16];
                snprintf(key, sizeof(key), "x_offset_%d", i);
                err = nvs_get_i32(handle, key, &touch_offsets[i][0]);
                if (err != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to read %s: %s", key, esp_err_to_name(err));
                    touch_offsets[i][0] = 0;
                }
                snprintf(key, sizeof(key), "y_offset_%d", i);
                err = nvs_get_i32(handle, key, &touch_offsets[i][1]);
                if (err != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to read %s: %s", key, esp_err_to_name(err));
                    touch_offsets[i][1] = 0;
                }
                ESP_LOGI(TAG, "Loaded offsets for rotation %d: x=%" PRId32 ", y=%" PRId32, i, touch_offsets[i][0], touch_offsets[i][1]);
            }
            nvs_close(handle);
        } else {
            ESP_LOGW(TAG, "No calibration data found in NVS, using defaults");
        }
    }

    indev_ = lv_indev_create();
    lv_indev_set_type(indev_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_, [](lv_indev_t* indev, lv_indev_data_t* data) {
        auto* touch = static_cast<CST820Touch*>(lv_indev_get_user_data(indev));
        touch->read_input(data);
    });
    lv_indev_set_user_data(indev_, this);
    lv_indev_set_display(indev_, display);
    display_ = display;
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
        is_pressed_ = false;
        return false;
    }

    uint8_t finger_num = touch_data[1];
    if (finger_num > 0) {
        // Raw hardware coordinates (240x320)
        uint16_t x = (touch_data[2] << 8) | touch_data[3];
        uint16_t y = (touch_data[4] << 8) | touch_data[5];

        // Transform coordinates based on display rotation
        lv_display_rotation_t rotation = lv_display_get_rotation(display_);
        int32_t logical_x = 0, logical_y = 0;  // Initialize to 0 to avoid uninitialized use

        ESP_LOGI(TAG, "Raw touch: x=%" PRIu16 ", y=%" PRIu16 ", rotation=%d", x, y, rotation);

        switch (rotation) {
            case LV_DISPLAY_ROTATION_90:
                // Landscape, USB on Right: (0, 0) top-left, (319, 239) bottom-right
                logical_x = 319 - (x * 319) / 239;  // Scale to logical width of 320 and invert
                logical_y = 239 - (y * 239) / 319;  // Scale to logical height of 240, invert
                break;

            case LV_DISPLAY_ROTATION_270:
                // Landscape Flipped, USB on Left: (0, 0) top-right, (319, 239) bottom-left
                logical_x = (x * 319) / 239;              // Scale to logical width of 320
                logical_y = 239 - (y * 239) / 319;        // Scale to logical height of 240 and invert
                break;

            case LV_DISPLAY_ROTATION_180:
                // Portrait Flipped, USB on Top: (0, 0) bottom-left, (239, 319) top-right
                logical_x = 239 - x;  // Invert x to match display
                logical_y = 319 - y;  // Invert y to match display
                break;

            case LV_DISPLAY_ROTATION_0:
                // Portrait, USB on Bottom: (0, 0) top-left, (239, 319) bottom-right
                logical_x = 239 - x;  // Invert x to match display
                logical_y = y;        // y matches directly
                break;
        }

        // Apply calibration offsets
        logical_x += touch_offsets[rotation][0];
        logical_y += touch_offsets[rotation][1];

        // Clamp coordinates to logical display bounds
        int32_t max_x = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270) ? 319 : 239;
        int32_t max_y = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270) ? 239 : 319;
        if (logical_x < 0) logical_x = 0;
        if (logical_x > max_x) logical_x = max_x;
        if (logical_y < 0) logical_y = 0;
        if (logical_y > max_y) logical_y = max_y;

        // Click filtering with time-based logic
        if (!is_pressed_) {
            // First press, store initial coordinates and timestamp
            last_x_ = logical_x;
            last_y_ = logical_y;
            touch_start_time_ = esp_timer_get_time() / 1000;  // Convert microseconds to milliseconds
            is_pressed_ = true;
        } else {
            // Check elapsed time since touch start
            uint32_t current_time = esp_timer_get_time() / 1000;  // Convert microseconds to milliseconds
            uint32_t touch_duration = current_time - touch_start_time_;

            if (touch_duration < 200) {  // 200ms threshold for a tap
                // Within tap duration, keep coordinates fixed to ensure a click
                logical_x = last_x_;
                logical_y = last_y_;
            } else {
                // After tap duration, allow dragging if movement is significant
                int32_t delta_x = logical_x - last_x_;
                int32_t delta_y = logical_y - last_y_;
                int32_t distance = delta_x * delta_x + delta_y * delta_y;  // Approximate distance squared
                if (distance > 625) {  // Threshold: ~25 pixels
                    last_x_ = logical_x;
                    last_y_ = logical_y;
                } else {
                    // Movement too small, keep last coordinates to prevent scrolling
                    logical_x = last_x_;
                    logical_y = last_y_;
                }
            }
        }

        ESP_LOGI(TAG, "Transformed touch: logical x=%" PRId32 ", y=%" PRId32, logical_x, logical_y);

        data->point.x = logical_x;
        data->point.y = logical_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        is_pressed_ = false;
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
