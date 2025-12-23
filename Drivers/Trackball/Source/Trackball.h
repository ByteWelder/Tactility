#pragma once

#include <driver/gpio.h>
#include <lvgl.h>

namespace driver::trackball {

/**
 * @brief Trackball configuration structure
 */
struct TrackballConfig {
    gpio_num_t pinRight;      // Right direction GPIO
    gpio_num_t pinUp;         // Up direction GPIO
    gpio_num_t pinLeft;       // Left direction GPIO
    gpio_num_t pinDown;       // Down direction GPIO
    gpio_num_t pinClick;      // Click/select button GPIO
    uint8_t movementStep;     // Pixels to move per trackball event (default: 10)
};

/**
 * @brief Initialize trackball as LVGL input device
 * @param config Trackball GPIO configuration
 * @return LVGL input device pointer, or nullptr on failure
 */
lv_indev_t* init(const TrackballConfig& config);

/**
 * @brief Deinitialize trackball
 */
void deinit();

/**
 * @brief Set movement step size
 * @param step Pixels to move per trackball event
 */
void setMovementStep(uint8_t step);

/**
 * @brief Enable or disable trackball input processing
 * @param enabled Boolean value to enable or disable
 */
void setEnabled(bool enabled);

}
