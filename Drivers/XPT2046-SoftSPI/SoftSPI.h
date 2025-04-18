#pragma once

#include <driver/gpio.h>

/**
 * @brief Software SPI implementation for ESP32 platform
 * 
 * This class implements a bit-banged SPI interface using GPIO pins. It's useful
 * when hardware SPI is unavailable or when different pin configurations are needed.
 */
class SoftSPI {
public:
    /**
     * @brief Configuration for the SoftSPI instance
     */
    struct Config {
        gpio_num_t miso_pin;  ///< Master In Slave Out pin
        gpio_num_t mosi_pin;  ///< Master Out Slave In pin
        gpio_num_t sck_pin;   ///< Serial Clock pin
        gpio_num_t cs_pin;    ///< Chip Select pin
        uint32_t delay_us = 10; ///< Delay between signal transitions in microseconds
    };

    /**
     * @brief Construct a new SoftSPI instance
     * 
     * @param config The SoftSPI configuration
     */
    explicit SoftSPI(const Config& config);

    /**
     * @brief Initialize the SPI pins
     * 
     * @return true if initialization was successful
     * @return false otherwise
     */
    bool begin();

    /**
     * @brief Transfer one byte over SPI and read response
     * 
     * @param data Byte to send
     * @return uint8_t Received byte
     */
    uint8_t transfer(uint8_t data);

    /**
     * @brief Set the chip select pin to low (active)
     */
    void cs_low();

    /**
     * @brief Set the chip select pin to high (inactive)
     */
    void cs_high();

private:
    gpio_num_t miso_pin_;
    gpio_num_t mosi_pin_;
    gpio_num_t sck_pin_;
    gpio_num_t cs_pin_;
    uint32_t delay_us_;
};
