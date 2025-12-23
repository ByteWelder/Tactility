#pragma once

#include <driver/i2c.h>
#include <cstdint>

namespace driver::keyboardbacklight {

/**
 * @brief Initialize keyboard backlight control
 * @param i2cPort I2C port number (I2C_NUM_0 or I2C_NUM_1)
 * @param slaveAddress I2C slave address (default 0x55 for T-Deck keyboard)
 * @return true if initialization succeeded
 */
bool init(i2c_port_t i2cPort, uint8_t slaveAddress = 0x55);

/**
 * @brief Set keyboard backlight brightness
 * @param brightness Brightness level (0-255, 0=off, 255=max)
 * @return true if command succeeded
 */
bool setBrightness(uint8_t brightness);

/**
 * @brief Set default keyboard backlight brightness for ALT+B toggle
 * @param brightness Default brightness level (30-255)
 * @return true if command succeeded
 */
bool setDefaultBrightness(uint8_t brightness);

/**
 * @brief Get current keyboard backlight brightness
 * @return Current brightness level (0-255)
 */
uint8_t getBrightness();

}
