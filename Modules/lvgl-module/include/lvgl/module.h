// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The LVGL module instance.
 */
extern struct Module lvgl_module;

/**
 * @brief Configuration for the LVGL module.
 */
struct LvglModuleConfig {
    /**
     * @brief Callback invoked when the LVGL task starts.
     * Use this to add devices (e.g. displays, pointers), start services, create widgets, etc.
     */
    void (*on_start)(void);

    /**
     * @brief Callback invoked when the LVGL task stops.
     * Use this to remove devices, stop services, etc.
     */
    void (*on_stop)(void);

    /** @brief Priority of the LVGL task. */
    int task_priority;

    /** @brief Stack size of the LVGL task in bytes. */
    int task_stack_size;

#ifdef ESP_PLATFORM
    /** @brief CPU affinity of the LVGL task (ESP32 specific). */
    int task_affinity;
#endif
};

/**
 * @brief Configures the LVGL module.
 * @warning Must not be called when module is started.
 * @param config The configuration to apply.
 */
void lvgl_module_configure(struct LvglModuleConfig config);

#ifdef __cplusplus
}
#endif
