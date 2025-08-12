/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
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
* @brief Prototype of ssd1681 driver event callback
*
* @param[in] handle    esp_lcd_panel_handle_t esp_lcd driver handle
* @param[in] edata     reserved
* @param[in] user_data User registered context, registered in `epaper_panel_register_event_callbacks()`
*
* @return Whether a high priority task is woken up by this function
*/
typedef bool (*esp_lcd_epaper_panel_cb_t)(const esp_lcd_panel_handle_t handle, const void *edata, void *user_data);

/**
 * @brief Type of ssd1681 e-paper callbacks
 */
typedef struct {
    esp_lcd_epaper_panel_cb_t on_epaper_refresh_done;  /*!< Callback invoked when e-paper refresh finishes */
} epaper_panel_callbacks_t;

/**
 * @brief Enum of colors available of ssd1681 e-paper
 *        Some of the ssd1681 e-paper panels support not only black pixels but also red pixels.
 *        Use this enum to select the color of the bitmap you want to display.
 *        Need be set using `epaper_panel_set_bitmap_color()` before calling `epaper_panel_draw_bitmap()`.
 * @note Default to `SSD1681_EPAPER_BITMAP_BLACK` if not set.
 */
typedef enum {
    SSD1681_EPAPER_BITMAP_BLACK, /*!< Draw the bitmap in black */
    SSD1681_EPAPER_BITMAP_RED    /*!< Draw the bitmap in red */
} esp_lcd_gdeq_bitmap_color_t;

/**
 * @brief Create LCD panel for model ssd1681 e-Paper
 * @attention
 *        Need to call `gpio_install_isr_service()` before calling this function.
 * @param[in] io LCD panel IO handle
 * @param[in] panel_dev_config general panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 *          - ESP_ERR_INVALID_ARG   if parameter is invalid
 *          - ESP_ERR_NO_MEM        if out of memory
 *          - ESP_OK                on success
 */
esp_err_t esp_lcd_new_panel_gdeq031t10(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                                    esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief Refresh the e-Paper
 *
 * @note This function is called automatically in `draw_bitmap()` function.
 *       This function will return right after the refresh commands finish transmitting.
 * @attention
 *       If you want to call this function, you have to wait manually until the BUSY pin goes LOW
 *       before calling other functions that interacts with the e-paper.
 *
 * @param[in] panel LCD panel handle
 * @return
 *          - ESP_ERR_INVALID_ARG   if parameter is invalid
 *          - ESP_OK                on success
 */
esp_err_t epaper_panel_refresh_screen(esp_lcd_panel_t *panel, bool fast, bool full);

/**
 * @brief Set the color of the next bitmap
 *
 * @note The SSD1681 has two separate vrams. One for black and the other for red(or maybe some other color).
 *       Call this function to set which vram the next bitmap would write into.
 *       If you set `epaper_panel_invert_color()` to false, then 0 means WHITE.
 *
 * @param[in] panel LCD panel handle
 * @param[in] color a enum value, SSD1681_EPAPER_BITMAP_BLACK or SSD1681_EPAPER_BITMAP_RED
 * @return ESP_OK                on success
 */
esp_err_t epaper_panel_set_bitmap_color(esp_lcd_panel_t *panel, esp_lcd_gdeq_bitmap_color_t color);

/**
 * @brief Set the callback function
 *
 * @note The callback function `on_epaper_refresh_done` will only be called right after a `draw_bitmap()` caused refresh finishes.
 * @note You could set the `epaper_panel_callbacks_t->esp_lcd_epaper_panel_cb_t` to NULL to unregister the callback.
 * @attention
 *       The callback function `on_epaper_refresh_done` will be called in isr context, so please keep it as neat as possible.
 *
 * @param[in] panel LCD panel handle
 * @param[in] cbs callback functions
 * @param[in] user_ctx arg to be passed to your callback function
 * @return ESP_OK                on success
 *         ESP_ERR_INVALID_ARG   if parameter is invalid
 */
esp_err_t epaper_panel_register_event_callbacks(esp_lcd_panel_t *panel, epaper_panel_callbacks_t *cbs, void *user_ctx);

/**
 * @brief Set a custom waveform lut
 *
 * @note Set a custom waveform lut for your e-paper panel.
 * @note You do not have to call this function because e-paper panels usually have waveform lut built-in.
 * @note You could call `disp_on_off(panel_handle, true)` to reset the waveform LUT to the built-in one
 *
 * @param[in] panel LCD panel handle
 * @param[in] lut your custom lut array
 * @param[in] size size of your lut array, make sure it is SSD1681_LUT_SIZE bytes
 * @return  ESP_OK                on success
 *          ESP_ERR_INVALID_ARG   if parameter is invalid
 */
esp_err_t epaper_panel_set_custom_lut(esp_lcd_panel_t *panel, uint8_t *lut, size_t size);


#ifdef __cplusplus
}
#endif
