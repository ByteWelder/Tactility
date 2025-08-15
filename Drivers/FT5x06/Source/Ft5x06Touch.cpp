#include "Ft5x06Touch.h"

#include <Tactility/Log.h>

#include <esp_lcd_touch_ft5x06.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>

#define TAG "ft5x06"

bool Ft5x06Touch::createIoHandle(esp_lcd_panel_io_handle_t& outHandle) {
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    return esp_lcd_new_panel_io_i2c(configuration->port, &io_config, &outHandle) == ESP_OK;
}

bool Ft5x06Touch::createTouchHandle(esp_lcd_panel_io_handle_t ioHandle, const esp_lcd_touch_config_t& configuration, esp_lcd_touch_handle_t& panelHandle) {
    return esp_lcd_touch_new_i2c_ft5x06(ioHandle, &configuration, &panelHandle) == ESP_OK;
}

esp_lcd_touch_config_t Ft5x06Touch::createEspLcdTouchConfig() {
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
            .swap_xy = configuration->swapXy,
            .mirror_x = configuration->mirrorX,
            .mirror_y = configuration->mirrorY,
        },
        .process_coordinates = nullptr,
        .interrupt_callback = nullptr,
        .user_data = nullptr,
        .driver_data = nullptr
    };
}
