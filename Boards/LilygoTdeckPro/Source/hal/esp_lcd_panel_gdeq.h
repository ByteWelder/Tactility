/**
 * This code is based on the esp_lcd_ssd1681 driver from https://github.com/espressif/esp-bsp/tree/master/components/lcd/esp_lcd_ssd1681
 * The esp_lcd_ssd1681 driver is copyrighted by "2023 Espressif Systems (Shanghai) CO LTD" and was distributed under Apache License:
 * https://github.com/espressif/esp-bsp/blob/master/components/lcd/esp_lcd_ssd1681/license.txt
 *
 * The gdeq driver is licensed under Tactility's GPL v2 license.
 */
#pragma once

#include <stdbool.h>
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create panel for display model gdeq031t10
 * @param[in] io panel IO handle
 * @param[in] panel_dev_config general panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 *          - ESP_ERR_INVALID_ARG   if parameter is invalid
 *          - ESP_ERR_NO_MEM        if out of memory
 *          - ESP_OK                on success
 */
esp_err_t esp_lcd_new_panel_gdeq031t10(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief Refresh the display
 *
 * @param[in] panel LCD panel handle
 * @param[in] fast whether to refresh fast or regularly
 * @param[in] full whether to do a full refresh (slower) or a partial one
 *
 * @return
 *          - ESP_ERR_INVALID_ARG   if parameter is invalid
 *          - ESP_OK                on success
 */
esp_err_t epaper_panel_refresh_screen(esp_lcd_panel_t *panel, bool fast, bool full);

#ifdef __cplusplus
}
#endif
