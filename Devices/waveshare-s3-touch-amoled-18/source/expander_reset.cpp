// SPDX-License-Identifier: Apache-2.0

// The Waveshare 1.8" board does not wire the AMOLED panel or touch reset lines to ESP32 GPIOs.
// They hang off a TCA9554 I2C expander (address 0x20), whose pins the board's own examples drive
// low then high (EXIO0, EXIO1, EXIO2 and EXIO6) before initializing the panel and touch
// controller. Without that pulse the SH8601 panel stays blank and the FT3168 touch NAKs every
// I2C transaction (the later "FT5x06 init failed").
//
// This runs from the board module's start(), which is before the kernel constructs and starts the
// devicetree's devices, so the reset is released ahead of the display/touch drivers' own init. It
// uses a temporary ESP-IDF I2C master (on a spare port, so it can't collide with the kernel's
// i2c0) and tears it down again, leaving the bus for i2c0 to set up afterwards.

#include <tactility/log.h>

#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {

constexpr auto* TAG = "ws-amoled18-expander";

constexpr uint8_t EXPANDER_ADDRESS = 0x20;

constexpr gpio_num_t I2C_SDA = GPIO_NUM_15;
constexpr gpio_num_t I2C_SCL = GPIO_NUM_14;

// TCA9554 register map (shared with the TCA9534).
constexpr uint8_t REG_OUTPUT = 0x01;
constexpr uint8_t REG_CONFIG = 0x03;

// EXIO0/1/2/6 drive the panel and touch reset lines.
constexpr uint8_t RESET_PINS = (uint8_t)((1u << 0) | (1u << 1) | (1u << 2) | (1u << 6));

bool write_register(i2c_master_dev_handle_t device, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = { reg, value };
    return i2c_master_transmit(device, buffer, sizeof(buffer), -1) == ESP_OK;
}

} // namespace

void waveshare_amoled18_expander_reset() {
    i2c_master_bus_config_t bus_config = {};
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.i2c_port = I2C_NUM_1; // spare port; i2c0 belongs to the kernel devicetree
    bus_config.sda_io_num = I2C_SDA;
    bus_config.scl_io_num = I2C_SCL;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus = nullptr;
    esp_err_t err = i2c_new_master_bus(&bus_config, &bus);
    if (err != ESP_OK) {
        LOG_W(TAG, "Failed to open I2C bus: %s", esp_err_to_name(err));
        return;
    }

    i2c_device_config_t device_config = {};
    device_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device_config.device_address = EXPANDER_ADDRESS;
    device_config.scl_speed_hz = 400000;

    i2c_master_dev_handle_t device = nullptr;
    err = i2c_master_bus_add_device(bus, &device_config, &device);
    if (err == ESP_OK) {
        // EXIO0/1/2/6 as outputs (config bit 0 = output), the rest left as inputs; then the
        // reset pulse itself.
        if (!write_register(device, REG_CONFIG, (uint8_t)~RESET_PINS) ||
            !write_register(device, REG_OUTPUT, 0x00)) {
            LOG_W(TAG, "Expander reset: initial writes failed");
        }
        vTaskDelay(pdMS_TO_TICKS(20));
        if (!write_register(device, REG_OUTPUT, RESET_PINS)) {
            LOG_W(TAG, "Expander reset: release write failed");
        } else {
            LOG_I(TAG, "Panel/touch reset released via TCA9554");
        }
        i2c_master_bus_rm_device(device);
    } else {
        LOG_W(TAG, "Failed to add expander device: %s", esp_err_to_name(err));
    }

    i2c_del_master_bus(bus);
}
