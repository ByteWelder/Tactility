/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_string.h
 *
 */

#ifndef LV_STRING_H
#define LV_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
//#include "../lv_conf_internal.h"
#include <stdint.h>
#include <stddef.h>
#include "lv_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Copies a block of memory from a source address to a destination address.
 * @param dst Pointer to the destination array where the content is to be copied.
 * @param src Pointer to the source of data to be copied.
 * @param len Number of bytes to copy.
 * @return Pointer to the destination array.
 * @note The function does not check for any overlapping of the source and destination memory blocks.
 */
void *lv_memcpy(void *dst, const void *src, size_t len);

/**
 * @brief Fills a block of memory with a specified value.
 * @param dst Pointer to the destination array to fill with the specified value.
 * @param v Value to be set. The value is passed as an int, but the function fills
 *          the block of memory using the unsigned char conversion of this value.
 * @param len Number of bytes to be set to the value.
 */
void lv_memset(void *dst, uint8_t v, size_t len);

/**
 * @brief Move a block of memory from source to destination
 * @param dst Pointer to the destination array where the content is to be copied.
 * @param src Pointer to the source of data to be copied.
 * @param len Number of bytes to copy
 * @return Pointer to the destination array.
 */
void *lv_memmove(void *dst, const void *src, size_t len);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_STRING_H*/
