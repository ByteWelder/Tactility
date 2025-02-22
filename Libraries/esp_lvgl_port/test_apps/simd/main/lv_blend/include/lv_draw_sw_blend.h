/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_draw_sw_blend.h
 *
 */

#ifndef LV_DRAW_SW_BLEND_H
#define LV_DRAW_SW_BLEND_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_style.h"
#include "lv_color.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    void *dest_buf;
    int32_t dest_w;
    int32_t dest_h;
    int32_t dest_stride;
    const lv_opa_t *mask_buf;
    int32_t mask_stride;
    lv_color_t color;
    lv_opa_t opa;
    bool use_asm;
} _lv_draw_sw_blend_fill_dsc_t;

typedef struct {
    void *dest_buf;
    int32_t dest_w;
    int32_t dest_h;
    int32_t dest_stride;
    const lv_opa_t *mask_buf;
    int32_t mask_stride;
    const void *src_buf;
    int32_t src_stride;
    lv_color_format_t src_color_format;
    lv_opa_t opa;
    lv_blend_mode_t blend_mode;
    bool use_asm;
} _lv_draw_sw_blend_image_dsc_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_SW_BLEND_H*/
