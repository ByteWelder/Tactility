/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#if !CONFIG_LV_DRAW_SW_ASM_CUSTOM
#warning "esp_lvgl_port_lv_blend.h included, but CONFIG_LV_DRAW_SW_ASM_CUSTOM not set. Assembly rendering not used"
#else

/*********************
 *      DEFINES
 *********************/

#ifndef LV_DRAW_SW_COLOR_BLEND_TO_ARGB8888
#define LV_DRAW_SW_COLOR_BLEND_TO_ARGB8888(dsc) \
    _lv_color_blend_to_argb8888_esp(dsc)
#endif

#ifndef LV_DRAW_SW_COLOR_BLEND_TO_RGB565
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565(dsc) \
    _lv_color_blend_to_rgb565_esp(dsc)
#endif


/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint32_t opa;
    void *dst_buf;
    uint32_t dst_w;
    uint32_t dst_h;
    uint32_t dst_stride;
    const void *src_buf;
    uint32_t src_stride;
    const lv_opa_t *mask_buf;
    uint32_t mask_stride;
} asm_dsc_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

extern int lv_color_blend_to_argb8888_esp(asm_dsc_t *asm_dsc);

static inline lv_result_t _lv_color_blend_to_argb8888_esp(_lv_draw_sw_blend_fill_dsc_t *dsc)
{
    asm_dsc_t asm_dsc = {
        .dst_buf = dsc->dest_buf,
        .dst_w = dsc->dest_w,
        .dst_h = dsc->dest_h,
        .dst_stride = dsc->dest_stride,
        .src_buf = &dsc->color,
    };

    return lv_color_blend_to_argb8888_esp(&asm_dsc);
}

extern int lv_color_blend_to_rgb565_esp(asm_dsc_t *asm_dsc);

static inline lv_result_t _lv_color_blend_to_rgb565_esp(_lv_draw_sw_blend_fill_dsc_t *dsc)
{
    asm_dsc_t asm_dsc = {
        .dst_buf = dsc->dest_buf,
        .dst_w = dsc->dest_w,
        .dst_h = dsc->dest_h,
        .dst_stride = dsc->dest_stride,
        .src_buf = &dsc->color,
    };

    return lv_color_blend_to_rgb565_esp(&asm_dsc);
}

#endif // CONFIG_LV_DRAW_SW_ASM_CUSTOM

#ifdef __cplusplus
} /*extern "C"*/
#endif
