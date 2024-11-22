/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP LVGL port display
 */

#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

#if LVGL_VERSION_MAJOR == 8
#include "esp_lvgl_port_compatibility.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Rotation configuration
 */
typedef struct {
    bool swap_xy;  /*!< LCD Screen swapped X and Y (in esp_lcd driver) */
    bool mirror_x; /*!< LCD Screen mirrored X (in esp_lcd driver) */
    bool mirror_y; /*!< LCD Screen mirrored Y (in esp_lcd driver) */
} lvgl_port_rotation_cfg_t;

/**
 * @brief Configuration display structure
 */
typedef struct {
    esp_lcd_panel_io_handle_t io_handle;    /*!< LCD panel IO handle */
    esp_lcd_panel_handle_t panel_handle;    /*!< LCD panel handle */
    uint32_t    buffer_size;    /*!< Size of the buffer for the screen in pixels */
    bool        double_buffer;  /*!< True, if should be allocated two buffers */
    uint32_t    trans_size;     /*!< Allocated buffer will be in SRAM to move framebuf */
    uint32_t    hres;           /*!< LCD display horizontal resolution */
    uint32_t    vres;           /*!< LCD display vertical resolution */
    bool        monochrome;     /*!< True, if display is monochrome and using 1bit for 1px */
    lvgl_port_rotation_cfg_t rotation;    /*!< Default values of the screen rotation */

    struct {
        unsigned int buff_dma: 1;    /*!< Allocated LVGL buffer will be DMA capable */
        unsigned int buff_spiram: 1; /*!< Allocated LVGL buffer will be in PSRAM */
        unsigned int sw_rotate: 1;   /*!< Use software rotation (slower) */
        unsigned int swap_bytes: 1;  /*!< Swap bytes in RGB656 (16-bit) before send to LCD driver */
    } flags;
} lvgl_port_display_cfg_t;

/**
 * @brief Add display handling to LVGL
 *
 * @note Allocated memory in this function is not free in deinit. You must call lvgl_port_remove_disp for free all memory!
 *
 * @param disp_cfg Display configuration structure
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_display_t *lvgl_port_add_disp(const lvgl_port_display_cfg_t *disp_cfg);

/**
 * @brief Remove display handling from LVGL
 *
 * @note Free all memory used for this display.
 *
 * @return
 *      - ESP_OK                    on success
 */
esp_err_t lvgl_port_remove_disp(lv_display_t *disp);

#ifdef __cplusplus
}
#endif
