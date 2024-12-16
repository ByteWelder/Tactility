/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_log.h
 *
 */

#ifndef LV_LOG_H
#define LV_LOG_H

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

/*Do nothing if `LV_USE_LOG 0`*/
#define _lv_log_add(level, file, line, ...)
#define LV_LOG_TRACE(...) do {}while(0)
#define LV_LOG_INFO(...) do {}while(0)
#define LV_LOG_WARN(...) do {}while(0)
#define LV_LOG_ERROR(...) do {}while(0)
#define LV_LOG_USER(...) do {}while(0)
#define LV_LOG(...) do {}while(0)


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_LOG_H*/
