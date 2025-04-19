#pragma once

#include <memory>
#include <esp_lcd_touch.h>
#include <lvgl.h>
#include "SoftSPI.h"

/**
 * @brief XPT2046 touchscreen controller driver using software SPI
 * 
 * This class provides a driver for the XPT2046 resistive touchscreen controller
 * using a software SPI implementation for communication. It integrates with the
 * ESP-IDF esp_lcd_touch API and provides LVGL integration.
 */
class XPT2046_SoftSPI {
public:
    /**
     * @brief XPT2046 configuration structure
     */
    struct Config {
        // SPI connection pins
        gpio_num_t cs_pin;    ///< Chip Select pin
        gpio_num_t int_pin;   ///< Interrupt pin (optional, can be GPIO_NUM_NC)
        gpio_num_t miso_pin;  ///< MISO pin
        gpio_num_t mosi_pin;  ///< MOSI pin
        gpio_num_t sck_pin;   ///< SCK pin
        
        // Screen dimensions and behavior
        uint16_t x_max;       ///< Maximum X coordinate value
        uint16_t y_max;       ///< Maximum Y coordinate value
        
        // Calibration values
        uint16_t x_min_raw;   ///< Minimum raw X value from ADC
        uint16_t x_max_raw;   ///< Maximum raw X value from ADC
        uint16_t y_min_raw;   ///< Minimum raw Y value from ADC 
        uint16_t y_max_raw;   ///< Maximum raw Y value from ADC
        
        // Orientation flags
        bool swap_xy;         ///< Swap X and Y coordinates
        bool mirror_x;        ///< Mirror X coordinates
        bool mirror_y;        ///< Mirror Y coordinates
        
        // Optional Z-axis threshold (minimum pressure to register a touch)
        uint16_t z_threshold = 30;

        // SoftSPI timing configuration
        uint32_t spi_delay_us = 10; ///< Delay between SPI signal transitions (default 10us)
        uint32_t spi_post_command_delay_us = 2; ///< Delay after sending command before reading (default 2us)
    };

    /**
     * @brief Create a new XPT2046 driver instance
     * 
     * @param config Driver configuration
     * @return std::unique_ptr<XPT2046_SoftSPI> Unique pointer to driver instance or nullptr on failure
     */
    static std::unique_ptr<XPT2046_SoftSPI> create(const Config& config);
    
    /**
     * @brief Destroy the driver instance and clean up resources
     */
    ~XPT2046_SoftSPI();

    /**
     * @brief Get the ESP LCD touch handle for this driver
     * 
     * @return esp_lcd_touch_handle_t Handle that can be used with ESP-IDF API
     */
    esp_lcd_touch_handle_t get_handle() const { return handle_; }
    
    /**
     * @brief Get the LVGL input device for this touchscreen
     * 
     * @return lv_indev_t* LVGL input device pointer
     */
    lv_indev_t* get_lvgl_indev() const { return indev_; }
    
    /**
     * @brief Get raw touch coordinates from the controller
     * 
     * This function reads the raw ADC values from the touchscreen controller
     * without any calibration or filtering applied.
     * 
     * @param[out] x Raw X coordinate (0-4095) or 0 if no touch
     * @param[out] y Raw Y coordinate (0-4095) or 0 if no touch
     */
    void get_raw_touch(uint16_t& x, uint16_t& y);
    bool self_test();
    
    /**
     * @brief Read touch data from the controller
     * 
     * This function reads the current touch state from the controller and
     * performs filtering and scaling. The results are stored internally
     * and can be retrieved with get_xy().
     * 
     * @return esp_err_t ESP_OK on success
     */
    esp_err_t read_touch_data();

private:
    /**
     * @brief XPT2046 register commands
     */
    enum Command : uint8_t {
        READ_X = 0xD0 | 0x01,  // X position + power down between conversions
        READ_Y = 0x90 | 0x01,  // Y position + power down between conversions
        READ_Z1 = 0xB0 | 0x01, // Z1 position + power down between conversions
        READ_Z2 = 0xC0 | 0x01  // Z2 position + power down between conversions
    };

    /**
     * @brief Private constructor, use create() instead
     */
    explicit XPT2046_SoftSPI(const Config& config);
    
    /**
     * @brief Initialize the driver
     * 
     * @return true on success, false on failure
     */
    bool init();
    
    /**
     * @brief ESP-IDF touch callback for reading data
     */
    static esp_err_t read_data(esp_lcd_touch_handle_t tp);
    
    /**
     * @brief ESP-IDF touch callback for getting coordinates
     */
    static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t* x, uint16_t* y,
                      uint16_t* strength, uint8_t* point_num, uint8_t max_point_num);
    
    /**
     * @brief ESP-IDF touch callback for deleting the driver
     */
    static esp_err_t del(esp_lcd_touch_handle_t tp);
    
    /**
     * @brief LVGL input device read callback
     */
    static void lvgl_read_cb(lv_indev_t* indev, lv_indev_data_t* data);
    
    /**
     * @brief Read a register from the XPT2046
     * 
     * @param command Command/register to read
     * @param value Pointer to store the result
     * @return esp_err_t ESP_OK on success
     */
    esp_err_t read_register(uint8_t command, uint16_t* value);

    esp_lcd_touch_handle_t handle_;        ///< ESP LCD Touch handle
    lv_indev_t* indev_;                   ///< LVGL input device
    gpio_num_t cs_pin_;                   ///< Chip select pin
    gpio_num_t int_pin_;                  ///< Interrupt pin
    std::unique_ptr<SoftSPI> spi_;        ///< SoftSPI interface
    uint16_t z_threshold_;                ///< Z threshold value
    Config config_;                       ///< Driver configuration
};
