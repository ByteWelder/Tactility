/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_assert.h
 *
 */

#ifndef LV_ASSERT_H
#define LV_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_log.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/
#define LV_ASSERT_HANDLER while(1);   /*Halt by default*/

#define LV_ASSERT(expr)                                        \
    do {                                                       \
        if(!(expr)) {                                          \
            LV_LOG_ERROR("Asserted at expression: %s", #expr); \
            LV_ASSERT_HANDLER                                  \
        }                                                      \
    } while(0)

/*-----------------
 * ASSERTS
 *-----------------*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_ASSERT_H*/
