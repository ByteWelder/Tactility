#include "CST820Touch.h"
#include "esp_log.h"

static const char *TAG = "Cst820Touch";

Cst820Touch::Cst820Touch(std::unique_ptr<Configuration> config)
    : config_(std::move(config)) {}

bool Cst820Touch::read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    uint8_t touch_data[6];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x01, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, touch_data, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(config_->i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(err));
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

    uint8_t finger_num = touch_data[1];
    if (finger_num > 0) {
        uint16_t x = (touch_data[2] << 8) | touch_data[3];
        uint16_t y = (touch_data[4] << 8) | touch_data[5];
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }

    return false; // No more data to read
}

std::shared_ptr<tt::hal::touch::TouchDevice> create_cst820_touch() {
    auto configuration = std::make_unique<Cst820Touch::Configuration>(
        I2C_NUM_0,
        cyd_2432s022c::HORIZONTAL_RESOLUTION,
        cyd_2432s022c::VERTICAL_RESOLUTION
    );

    return std::make_shared<Cst820Touch>(std::move(configuration));
}
