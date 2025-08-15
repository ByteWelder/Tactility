#include "Cst816Touch.h"

#include <esp_lcd_touch_cst816s.h>

bool Cst816sTouch::createIoHandle(esp_lcd_panel_io_handle_t& ioHandle) {
    constexpr esp_lcd_panel_io_i2c_config_t touch_io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    return esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)configuration->port, &touch_io_config, &ioHandle) == ESP_OK;
}

bool Cst816sTouch::createTouchHandle(esp_lcd_panel_io_handle_t ioHandle, const esp_lcd_touch_config_t& touchConfiguration, esp_lcd_touch_handle_t& touchHandle) {
    return esp_lcd_touch_new_i2c_cst816s(ioHandle, &touchConfiguration, &touchHandle) == ESP_OK;
}

esp_lcd_touch_config_t Cst816sTouch::createEspLcdTouchConfig() {
    return {
        .x_max = configuration->xMax,
        .y_max = configuration->yMax,
        .rst_gpio_num = configuration->pinReset,
        .int_gpio_num = configuration->pinInterrupt,
        .levels = {
            .reset = configuration->pinResetLevel,
            .interrupt = configuration->pinInterruptLevel,
        },
        .flags = {
            .swap_xy = configuration->swapXY,
            .mirror_x = configuration->mirrorX,
            .mirror_y = configuration->mirrorY,
        },
        .process_coordinates = nullptr,
        .interrupt_callback = nullptr,
        .user_data = nullptr,
        .driver_data = nullptr
    };
}
