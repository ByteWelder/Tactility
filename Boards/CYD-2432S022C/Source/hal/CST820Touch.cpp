#include "CST820Touch.h"
#include "CYD2432S022CConstants.h"
#include "esp_log.h"

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
    return true;
}

bool CST820Touch::stop() {
    ESP_LOGI(TAG, "Stopping CST820 touch...");
    if (indev_) {
        lv_indev_delete(indev_);
        indev_ = nullptr;
    }
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
        uint16_t x = (touch_data[2] << 8) | touch_data[3];
        uint16_t y = (touch_data[4] << 8) | touch_data[5];
        data->point.x = x;
        data->point.y = y;
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
