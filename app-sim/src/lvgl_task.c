#include "lvgl_task.h"

#include "lvgl_hal.h"
#include "tactility_core.h"
#include "thread.h"
#include "ui/lvgl_sync.h"
#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"

#define TAG "lvgl_task"

// Mutex for LVGL drawing
static QueueHandle_t lvgl_mutex = NULL;
static uint32_t task_max_sleep_ms = 10;
// Mutex for LVGL task state (to modify task_running state)
static QueueHandle_t task_mutex = NULL;
static bool task_running = false;

static bool task_lock(int timeout_ticks) {
    assert(task_mutex != NULL);
    return xSemaphoreTakeRecursive(task_mutex, timeout_ticks) == pdTRUE;
}

static void task_unlock() {
    assert(task_mutex != NULL);
    xSemaphoreGiveRecursive(task_mutex);
}

static void task_set_running(bool running) {
    assert(task_lock(configTICK_RATE_HZ / 100));
    task_running = running;
    task_unlock();
}

static bool task_is_running() {
    assert(task_lock(configTICK_RATE_HZ / 100));
    bool result = task_running;
    task_unlock();
    return result;
}

static bool lvgl_lock(int timeout_ticks) {
    assert(lvgl_mutex != NULL);
    return xSemaphoreTakeRecursive(lvgl_mutex, timeout_ticks) == pdTRUE;
}

static void lvgl_unlock() {
    assert(lvgl_mutex != NULL);
    xSemaphoreGiveRecursive(lvgl_mutex);
}

static void lvgl_task_init() {
    if (lvgl_mutex == NULL) {
        TT_LOG_D(TAG, "init: creating lvgl mutex");
        lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    }

    if (task_mutex == NULL) {
        TT_LOG_D(TAG, "init: creating task mutex");
        task_mutex = xSemaphoreCreateRecursiveMutex();
    }

    tt_lvgl_sync_set(&lvgl_lock, &lvgl_unlock);
}

static void lvgl_task_deinit() {
    if (lvgl_mutex) {
        vSemaphoreDelete(lvgl_mutex);
        lvgl_mutex = NULL;
    }
    if (task_mutex) {
        vSemaphoreDelete(task_mutex);
        task_mutex = NULL;
    }
#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    lv_deinit();
#endif
}

void lvgl_task(TT_UNUSED void* arg) {
    lvgl_hal_init();
    lvgl_task_init();

    uint32_t task_delay_ms = task_max_sleep_ms;

    task_set_running(true);

    while (task_is_running()) {
        if (lvgl_lock(0)) {
            task_delay_ms = lv_timer_handler();
            lvgl_unlock();
        }
        if ((task_delay_ms > task_max_sleep_ms) || (1 == task_delay_ms)) {
            task_delay_ms = task_max_sleep_ms;
        } else if (task_delay_ms < 1) {
            task_delay_ms = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }

    lvgl_task_deinit();
    vTaskDelete(NULL);
}

bool lvgl_is_ready() {
    return task_running;
}

void lvgl_interrupt() {
    tt_check(lvgl_lock(TtWaitForever));
    task_set_running(false); // interrupt task with boolean as flag
    lvgl_unlock();
}
