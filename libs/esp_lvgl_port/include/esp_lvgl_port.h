/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP LVGL port
 */

#pragma once

#include "esp_err.h"
#include "esp_lvgl_port_button.h"
#include "esp_lvgl_port_compatibility.h"
#include "esp_lvgl_port_disp.h"
#include "esp_lvgl_port_knob.h"
#include "esp_lvgl_port_touch.h"
#include "esp_lvgl_port_usbhid.h"

#if LVGL_VERSION_MAJOR == 8
#include "esp_lvgl_port_compatibility.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init configuration structure
 */
typedef struct {
    int task_priority;      /*!< LVGL task priority */
    int task_stack;         /*!< LVGL task stack size */
    int task_affinity;      /*!< LVGL task pinned to core (-1 is no affinity) */
    int task_max_sleep_ms;  /*!< Maximum sleep in LVGL task */
    int timer_period_ms;    /*!< LVGL timer tick period in ms */
} lvgl_port_cfg_t;

/**
 * @brief LVGL port configuration structure
 *
 */
#define ESP_LVGL_PORT_INIT_CONFIG() \
    {                               \
        .task_priority = 4,       \
        .task_stack = 6144,       \
        .task_affinity = -1,      \
        .task_max_sleep_ms = 500, \
        .timer_period_ms = 5,     \
    }

/**
 * @brief Initialize LVGL portation
 *
 * @note This function initialize LVGL and create timer and task for LVGL right working.
 *
 * @return
 *      - ESP_OK                    on success
 *      - ESP_ERR_INVALID_ARG       if some of the create_args are not valid
 *      - ESP_ERR_INVALID_STATE     if esp_timer library is not initialized yet
 *      - ESP_ERR_NO_MEM            if memory allocation fails
 */
esp_err_t lvgl_port_init(const lvgl_port_cfg_t *cfg);

/**
 * @brief Deinitialize LVGL portation
 *
 * @note This function deinitializes LVGL and stops the task if running.
 * Some deinitialization will be done after the task will be stopped.
 *
 * @return
 *      - ESP_OK                    on success
 */
esp_err_t lvgl_port_deinit(void);

/**
 * @brief Take LVGL mutex
 *
 * @param timeout_ms Timeout in [ms]. 0 will block indefinitely.
 * @return
 *      - true  Mutex was taken
 *      - false Mutex was NOT taken
 */
bool lvgl_port_lock(uint32_t timeout_ms);

/**
 * @brief Give LVGL mutex
 *
 */
void lvgl_port_unlock(void);

/**
 * @brief Notify LVGL, that data was flushed to LCD display
 *
 * @note It should be used only when not called inside LVGL port (more in README).
 *
 * @param disp          LVGL display handle (returned from lvgl_port_add_disp)
 */
void lvgl_port_flush_ready(lv_display_t *disp);

/**
 * @brief Stop lvgl task
 *
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if the timer is not running
 */
esp_err_t lvgl_port_stop(void);

/**
 * @brief Resume lvgl task
 *
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if the timer is not running
 */
esp_err_t lvgl_port_resume(void);

#ifdef __cplusplus
}
#endif
