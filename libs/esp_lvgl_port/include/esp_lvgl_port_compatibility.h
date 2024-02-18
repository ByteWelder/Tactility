/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP LVGL port compatibility
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Backward compatibility with LVGL 8
 */
typedef lv_disp_t lv_display_t;

#ifdef __cplusplus
}
#endif
