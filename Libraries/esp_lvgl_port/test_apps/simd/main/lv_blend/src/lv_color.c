/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_color.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_color.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_color32_t lv_color_to_32(lv_color_t color, lv_opa_t opa)
{
    lv_color32_t c32;
    c32.red = color.red;
    c32.green = color.green;
    c32.blue = color.blue;
    c32.alpha = opa;
    return c32;
}

uint16_t lv_color_to_u16(lv_color_t color)
{
    return ((color.red & 0xF8) << 8) + ((color.green & 0xFC) << 3) + ((color.blue & 0xF8) >> 3);
}

uint32_t lv_color_to_u32(lv_color_t color)
{
    return (uint32_t)((uint32_t)0xff << 24) + (color.red << 16) + (color.green << 8) + (color.blue);
}
