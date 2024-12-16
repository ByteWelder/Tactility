/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_color.h
 *
 */

#ifndef LV_COLOR_H
#define LV_COLOR_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "stdint.h"
#include "stdbool.h"
#include "sdkconfig.h"

/*********************
 *      DEFINES
 *********************/
#define LV_ATTRIBUTE_FAST_MEM

#ifndef LV_COLOR_MIX_ROUND_OFS
#ifdef CONFIG_LV_COLOR_MIX_ROUND_OFS
#define LV_COLOR_MIX_ROUND_OFS CONFIG_LV_COLOR_MIX_ROUND_OFS
#else
#define LV_COLOR_MIX_ROUND_OFS  0
#endif
#endif

/**
 * Opacity percentages.
 */

typedef enum {
    LV_OPA_TRANSP = 0,
    LV_OPA_0      = 0,
    LV_OPA_10     = 25,
    LV_OPA_20     = 51,
    LV_OPA_30     = 76,
    LV_OPA_40     = 102,
    LV_OPA_50     = 127,
    LV_OPA_60     = 153,
    LV_OPA_70     = 178,
    LV_OPA_80     = 204,
    LV_OPA_90     = 229,
    LV_OPA_100    = 255,
    LV_OPA_COVER  = 255,
} lv_opa_t;

#define LV_OPA_MIN 2    /*Opacities below this will be transparent*/
#define LV_OPA_MAX 253  /*Opacities above this will fully cover*/

#define LV_COLOR_FORMAT_GET_BPP(cf) (       \
                                            (cf) == LV_COLOR_FORMAT_I1 ? 1 :        \
                                            (cf) == LV_COLOR_FORMAT_A1 ? 1 :        \
                                            (cf) == LV_COLOR_FORMAT_I2 ? 2 :        \
                                            (cf) == LV_COLOR_FORMAT_A2 ? 2 :        \
                                            (cf) == LV_COLOR_FORMAT_I4 ? 4 :        \
                                            (cf) == LV_COLOR_FORMAT_A4 ? 4 :        \
                                            (cf) == LV_COLOR_FORMAT_L8 ? 8 :        \
                                            (cf) == LV_COLOR_FORMAT_A8 ? 8 :        \
                                            (cf) == LV_COLOR_FORMAT_I8 ? 8 :        \
                                            (cf) == LV_COLOR_FORMAT_AL88 ? 16 :     \
                                            (cf) == LV_COLOR_FORMAT_RGB565 ? 16 :   \
                                            (cf) == LV_COLOR_FORMAT_RGB565A8 ? 16 : \
                                            (cf) == LV_COLOR_FORMAT_ARGB8565 ? 24 : \
                                            (cf) == LV_COLOR_FORMAT_RGB888 ? 24 :   \
                                            (cf) == LV_COLOR_FORMAT_ARGB8888 ? 32 : \
                                            (cf) == LV_COLOR_FORMAT_XRGB8888 ? 32 : \
                                            0                                       \
                                    )

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} lv_color_t;

typedef struct {
    uint16_t blue : 5;
    uint16_t green : 6;
    uint16_t red : 5;
} lv_color16_t;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
} lv_color32_t;

typedef struct {
    uint16_t h;
    uint8_t s;
    uint8_t v;
} lv_color_hsv_t;

typedef struct {
    uint8_t lumi;
    uint8_t alpha;
} lv_color16a_t;

typedef enum {
    LV_COLOR_FORMAT_UNKNOWN           = 0,

    LV_COLOR_FORMAT_RAW               = 0x01,
    LV_COLOR_FORMAT_RAW_ALPHA         = 0x02,

    /*<=1 byte (+alpha) formats*/
    LV_COLOR_FORMAT_L8                = 0x06,
    LV_COLOR_FORMAT_I1                = 0x07,
    LV_COLOR_FORMAT_I2                = 0x08,
    LV_COLOR_FORMAT_I4                = 0x09,
    LV_COLOR_FORMAT_I8                = 0x0A,
    LV_COLOR_FORMAT_A8                = 0x0E,

    /*2 byte (+alpha) formats*/
    LV_COLOR_FORMAT_RGB565            = 0x12,
    LV_COLOR_FORMAT_ARGB8565          = 0x13,   /**< Not supported by sw renderer yet. */
    LV_COLOR_FORMAT_RGB565A8          = 0x14,   /**< Color array followed by Alpha array*/
    LV_COLOR_FORMAT_AL88              = 0x15,   /**< L8 with alpha >*/

    /*3 byte (+alpha) formats*/
    LV_COLOR_FORMAT_RGB888            = 0x0F,
    LV_COLOR_FORMAT_ARGB8888          = 0x10,
    LV_COLOR_FORMAT_XRGB8888          = 0x11,

    /*Formats not supported by software renderer but kept here so GPU can use it*/
    LV_COLOR_FORMAT_A1                = 0x0B,
    LV_COLOR_FORMAT_A2                = 0x0C,
    LV_COLOR_FORMAT_A4                = 0x0D,

    /* reference to https://wiki.videolan.org/YUV/ */
    /*YUV planar formats*/
    LV_COLOR_FORMAT_YUV_START         = 0x20,
    LV_COLOR_FORMAT_I420              = LV_COLOR_FORMAT_YUV_START,  /*YUV420 planar(3 plane)*/
    LV_COLOR_FORMAT_I422              = 0x21,  /*YUV422 planar(3 plane)*/
    LV_COLOR_FORMAT_I444              = 0x22,  /*YUV444 planar(3 plane)*/
    LV_COLOR_FORMAT_I400              = 0x23,  /*YUV400 no chroma channel*/
    LV_COLOR_FORMAT_NV21              = 0x24,  /*YUV420 planar(2 plane), UV plane in 'V, U, V, U'*/
    LV_COLOR_FORMAT_NV12              = 0x25,  /*YUV420 planar(2 plane), UV plane in 'U, V, U, V'*/

    /*YUV packed formats*/
    LV_COLOR_FORMAT_YUY2              = 0x26,  /*YUV422 packed like 'Y U Y V'*/
    LV_COLOR_FORMAT_UYVY              = 0x27,  /*YUV422 packed like 'U Y V Y'*/

    LV_COLOR_FORMAT_YUV_END           = LV_COLOR_FORMAT_UYVY,

    /*Color formats in which LVGL can render*/
#if LV_COLOR_DEPTH == 8
    LV_COLOR_FORMAT_NATIVE            = LV_COLOR_FORMAT_L8,
    LV_COLOR_FORMAT_NATIVE_WITH_ALPHA = LV_COLOR_FORMAT_AL88,
#elif LV_COLOR_DEPTH == 16
    LV_COLOR_FORMAT_NATIVE            = LV_COLOR_FORMAT_RGB565,
    LV_COLOR_FORMAT_NATIVE_WITH_ALPHA = LV_COLOR_FORMAT_RGB565A8,
#elif LV_COLOR_DEPTH == 24
    LV_COLOR_FORMAT_NATIVE            = LV_COLOR_FORMAT_RGB888,
    LV_COLOR_FORMAT_NATIVE_WITH_ALPHA = LV_COLOR_FORMAT_ARGB8888,
#elif LV_COLOR_DEPTH == 32
    LV_COLOR_FORMAT_NATIVE            = LV_COLOR_FORMAT_XRGB8888,
    LV_COLOR_FORMAT_NATIVE_WITH_ALPHA = LV_COLOR_FORMAT_ARGB8888,
#endif
} lv_color_format_t;

/**********************
 * MACROS
 **********************/

#define LV_COLOR_MAKE(r8, g8, b8) {b8, g8, r8}

#define LV_OPA_MIX2(a1, a2) (((int32_t)(a1) * (a2)) >> 8)
#define LV_OPA_MIX3(a1, a2, a3) (((int32_t)(a1) * (a2) * (a3)) >> 16)

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an ARGB8888 color from RGB888 + alpha
 * @param color     an RGB888 color
 * @param opa       the alpha value
 * @return          the ARGB8888 color
 */
lv_color32_t lv_color_to_32(lv_color_t color, lv_opa_t opa);

/**
 * Convert am RGB888 color to RGB565 stored in `uint16_t`
 * @param color     and RGB888 color
 * @return          `color` as RGB565 on `uin16_t`
 */
uint16_t lv_color_to_u16(lv_color_t color);

/**
 * Convert am RGB888 color to XRGB8888 stored in `uint32_t`
 * @param color     and RGB888 color
 * @return          `color` as XRGB8888 on `uin32_t` (the alpha channel is always set to 0xFF)
 */
uint32_t lv_color_to_u32(lv_color_t color);

/**
 * Mix two RGB565 colors
 * @param c1        the first color (typically the foreground color)
 * @param c2        the second color  (typically the background color)
 * @param mix       0..255, or LV_OPA_0/10/20...
 * @return          mix == 0: c2
 *                  mix == 255: c1
 *                  mix == 128: 0.5 x c1 + 0.5 x c2
 */
static inline uint16_t LV_ATTRIBUTE_FAST_MEM lv_color_16_16_mix(uint16_t c1, uint16_t c2, uint8_t mix)
{
    if (mix == 255) {
        return c1;
    }
    if (mix == 0) {
        return c2;
    }
    if (c1 == c2) {
        return c1;
    }

    uint16_t ret;

    /* Source: https://stackoverflow.com/a/50012418/1999969*/
    mix = (uint32_t)((uint32_t)mix + 4) >> 3;

    /*0x7E0F81F = 0b00000111111000001111100000011111*/
    uint32_t bg = (uint32_t)(c2 | ((uint32_t)c2 << 16)) & 0x7E0F81F;
    uint32_t fg = (uint32_t)(c1 | ((uint32_t)c1 << 16)) & 0x7E0F81F;
    uint32_t result = ((((fg - bg) * mix) >> 5) + bg) & 0x7E0F81F;
    ret = (uint16_t)(result >> 16) | result;

    return ret;
}

/**
 * Check if two ARGB8888 color are equal
 * @param c1    the first color
 * @param c2    the second color
 * @return      true: equal
 */
static inline bool lv_color32_eq(lv_color32_t c1, lv_color32_t c2)
{
    return *((uint32_t *)&c1) == *((uint32_t *)&c2);
}

/**********************
 *      MACROS
 **********************/

#include "lv_color_op.h"

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_COLOR_H*/
