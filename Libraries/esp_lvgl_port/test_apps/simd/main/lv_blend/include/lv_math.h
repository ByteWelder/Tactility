/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_math.h
 *
 */

#ifndef LV_MATH_H
#define LV_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      MACROS
 **********************/
#define LV_MIN(a, b) ((a) < (b) ? (a) : (b))
#define LV_MIN3(a, b, c) (LV_MIN(LV_MIN(a,b), c))
#define LV_MIN4(a, b, c, d) (LV_MIN(LV_MIN(a,b), LV_MIN(c,d)))

#define LV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define LV_MAX3(a, b, c) (LV_MAX(LV_MAX(a,b), c))
#define LV_MAX4(a, b, c, d) (LV_MAX(LV_MAX(a,b), LV_MAX(c,d)))

#define LV_CLAMP(min, val, max) (LV_MAX(min, (LV_MIN(val, max))))

#define LV_ABS(x) ((x) > 0 ? (x) : (-(x)))
#define LV_UDIV255(x) (((x) * 0x8081U) >> 0x17)

#define LV_IS_SIGNED(t) (((t)(-1)) < ((t)0))
#define LV_UMAX_OF(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | (0xFULL << ((sizeof(t) * 8ULL) - 4ULL)))
#define LV_SMAX_OF(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | (0x7ULL << ((sizeof(t) * 8ULL) - 4ULL)))
#define LV_MAX_OF(t) ((unsigned long)(LV_IS_SIGNED(t) ? LV_SMAX_OF(t) : LV_UMAX_OF(t)))

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
