/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_draw_sw_blend_rgb888.h
 *
 */

#ifndef LV_DRAW_SW_BLEND_RGB888_H
#define LV_DRAW_SW_BLEND_RGB888_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_sw_blend.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void /* LV_ATTRIBUTE_FAST_MEM */ lv_draw_sw_blend_color_to_rgb888(_lv_draw_sw_blend_fill_dsc_t *dsc,
        uint32_t dest_px_size);

void /* LV_ATTRIBUTE_FAST_MEM */ lv_draw_sw_blend_image_to_rgb888(_lv_draw_sw_blend_image_dsc_t *dsc,
        uint32_t dest_px_size);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_SW_BLEND_RGB888_H*/
