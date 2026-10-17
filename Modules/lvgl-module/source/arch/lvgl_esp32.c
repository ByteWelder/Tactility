// SPDX-License-Identifier: Apache-2.0
#ifdef ESP_PLATFORM

#include <esp_heap_caps.h>
#include <esp_memory_utils.h>
#include <esp_lvgl_port.h>

#include <lvgl/lvgl.h>
#include <lvgl/module.h>
#include <lvgl/devices/keyboard_private.h>
#include <png_decoder.h>
#include <tactility/error.h>
#include <tactility/log.h>
#include <tactility/time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <string.h>

#define TAG "lvgl_esp32"

extern struct LvglModuleConfig lvgl_module_config;
extern void lvgl_devices_attach();
extern void lvgl_devices_detach();

static bool initialized = false;

void lvgl_lock(void) {
    if (!initialized) { return; }
    lvgl_port_lock(portMAX_DELAY);
}

bool lvgl_try_lock(uint32_t timeoutTicks) {
    if (!initialized) { return false; }
    // lvgl_port_lock expects milliseconds
    return lvgl_port_lock(timeoutTicks * portTICK_PERIOD_MS);
}

void lvgl_unlock(void) {
    if (!initialized) { return; }
    lvgl_port_unlock();
}

struct RunOnTaskRequest {
    void (*function)(void*);
    void* argument;
    SemaphoreHandle_t done;
};

// Runs inside the LVGL task, which holds the LVGL lock
static void run_on_task_callback(void* data) {
    struct RunOnTaskRequest* request = data;
    request->function(request->argument);
    xSemaphoreGive(request->done);
}

void lvgl_run_on_task(void (*function)(void*), void* argument) {
    const int stack_marker = 0;
    const bool is_lvgl_task = strcmp(pcTaskGetName(NULL), "taskLVGL") == 0;
    SemaphoreHandle_t done = NULL;
    // A stack in internal RAM can read files, and the LVGL task itself can always run the function
    if (initialized && !is_lvgl_task && !esp_ptr_internal(&stack_marker)) {
        done = xSemaphoreCreateBinary();
    }
    if (done == NULL) {
        lvgl_lock();
        function(argument);
        lvgl_unlock();
        return;
    }

    struct RunOnTaskRequest request = { .function = function, .argument = argument, .done = done };
    lvgl_lock();
    lv_async_call(run_on_task_callback, &request);
    lvgl_unlock();
    // The LVGL task may be asleep for a while otherwise
    lvgl_port_task_wake(LVGL_PORT_EVENT_USER, NULL);
    while (xSemaphoreTake(done, pdMS_TO_TICKS(1000)) != pdTRUE) {
        if (!initialized) {
            // LVGL stopped before it ran the function
            lv_async_call_cancel(run_on_task_callback, &request);
            break;
        }
    }
    vSemaphoreDelete(done);
}

error_t lvgl_arch_start() {
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = lvgl_module_config.task_priority,
        .task_stack = lvgl_module_config.task_stack_size,
        .task_affinity = lvgl_module_config.task_affinity,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        return ERROR_RESOURCE;
    }

    // We must have the state set to "initialized" so that the lvgl lock works
    // when we call listener functions. These functions could create new
    // devices and services. The latter might start adding widgets immediately.
    initialized = true;

    lvgl_lock();
    lvgl_png_decoder_init();
    lvgl_unlock();

    // Must exist before devices/services are attached below, since those can
    // immediately try to assign an indev to this group (e.g. USB HID input).
    lvgl_keyboard_on_start_lvgl();

    lvgl_devices_attach();

    if (lvgl_module_config.on_start) lvgl_module_config.on_start();

    return ERROR_NONE;
}

error_t lvgl_arch_stop() {
    if (lvgl_module_config.on_stop) lvgl_module_config.on_stop();

    lvgl_devices_detach();

    lvgl_keyboard_on_stop_lvgl();

    if (lvgl_port_deinit() != ESP_OK) {
        // Recreate what stop() above tore down, then call on_start again to recover
        lvgl_keyboard_on_start_lvgl();
        if (lvgl_module_config.on_start) lvgl_module_config.on_start();
        return ERROR_RESOURCE;
    }

    initialized = false;
    return ERROR_NONE;
}

#endif