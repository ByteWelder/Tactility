#include "lvgl_task.h"

#include "lvgl.h"
#include "log.h"
#include "lvgl_hal.h"
#include "tactility_core.h"
#include "thread.h"
#include "ui/lvgl_sync.h"

#include "FreeRTOS.h"
#include "semphr.h"

#define TAG "lvgl_task"

// Mutex for LVGL drawing
static QueueHandle_t lvgl_mutex = NULL;
static uint32_t task_max_sleep_ms = 10;
// Mutex for LVGL task state (to modify task_running state)
static QueueHandle_t task_mutex = NULL;
static bool task_running = false;

static void lvgl_task(void* arg);

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

bool lvgl_task_is_running() {
    assert(task_lock(configTICK_RATE_HZ / 100));
    bool result = task_running;
    task_unlock();
    return result;
}

static bool lvgl_lock(uint32_t timeout_ticks) {
    assert(lvgl_mutex != NULL);
    return xSemaphoreTakeRecursive(lvgl_mutex, timeout_ticks) == pdTRUE;
}

static void lvgl_unlock() {
    assert(lvgl_mutex != NULL);
    xSemaphoreGiveRecursive(lvgl_mutex);
}

void lvgl_task_interrupt() {
    tt_check(lvgl_lock(tt::TtWaitForever));
    task_set_running(false); // interrupt task with boolean as flag
    lvgl_unlock();
}

void lvgl_task_start() {
    TT_LOG_I(TAG, "lvgl task starting");

    if (lvgl_mutex == NULL) {
        TT_LOG_D(TAG, "init: creating lvgl mutex");
        lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    }

    if (task_mutex == NULL) {
        TT_LOG_D(TAG, "init: creating task mutex");
        task_mutex = xSemaphoreCreateRecursiveMutex();
    }

    tt::lvgl::sync_set(&lvgl_lock, &lvgl_unlock);

    // Create the main app loop, like ESP-IDF
    BaseType_t task_result = xTaskCreate(
        lvgl_task,
        "lvgl",
        8192,
        NULL,
        tt::ThreadPriorityHigh, // Should be higher than main app task
        NULL
    );

    tt_assert(task_result == pdTRUE);
}

static void lvgl_task(TT_UNUSED void* arg) {
    TT_LOG_I(TAG, "lvgl task started");

    lv_disp_t* display = lvgl_hal_init();

    uint32_t task_delay_ms = task_max_sleep_ms;

    task_set_running(true);

    while (lvgl_task_is_running()) {
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

    lv_disp_remove(display);

    vTaskDelete(NULL);
}

