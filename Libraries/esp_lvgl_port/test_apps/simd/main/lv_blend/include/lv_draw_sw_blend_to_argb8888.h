/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_draw_sw_blend_argb8888.h
 *
 */

#ifndef LV_DRAW_SW_BLEND_ARGB8888_H
#define LV_DRAW_SW_BLEND_ARGB8888_H

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

void /* LV_ATTRIBUTE_FAST_MEM */ lv_draw_sw_blend_color_to_argb8888(_lv_draw_sw_blend_fill_dsc_t *dsc);

void /* LV_ATTRIBUTE_FAST_MEM */ lv_draw_sw_blend_image_to_argb8888(_lv_draw_sw_blend_image_dsc_t *dsc);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_SW_BLEND_ARGB8888_H*/
