/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_types.h
 *
 */

#ifndef LV_TYPES_H
#define LV_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**********************
 *      TYPEDEFS
 **********************/

/**
 * LVGL error codes.
 */
typedef enum {
    LV_RESULT_INVALID = 0, /*Typically indicates that the object is deleted (become invalid) in the action
                      function or an operation was failed*/
    LV_RESULT_OK,      /*The object is valid (no deleted) after the action*/
} lv_result_t;

/**********************
 *      TYPEDEFS
 **********************/

typedef uintptr_t lv_uintptr_t;

/**********************
 *      MACROS
 **********************/

#define LV_UNUSED(x) ((void)x)

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_TYPES_H*/
