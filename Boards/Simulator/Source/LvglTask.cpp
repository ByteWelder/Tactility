#include "LvglTask.h"

#include "lvgl.h"
#include "Log.h"
#include "Thread.h"
#include "lvgl/LvglSync.h"

#include "Mutex.h"

#define TAG "lvgl_task"

// Mutex for LVGL drawing
static tt::Mutex lvgl_mutex(tt::Mutex::Type::Recursive);
static tt::Mutex task_mutex(tt::Mutex::Type::Recursive);

static uint32_t task_max_sleep_ms = 10;
// Mutex for LVGL task state (to modify task_running state)
static bool task_running = false;

static void lvgl_task(void* arg);

static bool task_lock(int timeout_ticks) {
    return task_mutex.acquire(timeout_ticks) == tt::TtStatusOk;
}

static void task_unlock() {
    task_mutex.release();
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

static bool lvgl_lock(uint32_t timeoutMillis) {
    return lvgl_mutex.acquire(pdMS_TO_TICKS(timeoutMillis)) == tt::TtStatusOk;
}

static void lvgl_unlock() {
    lvgl_mutex.release();
}

void lvgl_task_interrupt() {
    tt_check(task_lock(tt::TtWaitForever));
    task_set_running(false); // interrupt task with boolean as flag
    task_unlock();
}

void lvgl_task_start() {
    TT_LOG_I(TAG, "lvgl task starting");

    tt::lvgl::syncSet(&lvgl_lock, &lvgl_unlock);

    // Create the main app loop, like ESP-IDF
    BaseType_t task_result = xTaskCreate(
        lvgl_task,
        "lvgl",
        8192,
        nullptr,
        static_cast<UBaseType_t>(tt::Thread::Priority::High), // Should be higher than main app task
        nullptr
    );

    assert(task_result == pdTRUE);
}

lv_disp_t* displayHandle = nullptr;

static void lvgl_task(TT_UNUSED void* arg) {
    TT_LOG_I(TAG, "lvgl task started");

    /** Ideally. the display handle would be created during Simulator.start(),
     * but somehow that doesn't work. Waiting here from a ThreadFlag when that happens
     * also doesn't work. It seems that it must be called from this task. */
    displayHandle = lv_sdl_window_create(320, 240);
    lv_sdl_window_set_title(displayHandle, "Tactility");

    uint32_t task_delay_ms = task_max_sleep_ms;

    task_set_running(true);

    while (lvgl_task_is_running()) {
        if (lvgl_lock(10)) {
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

    lv_disp_remove(displayHandle);
    displayHandle = nullptr;
}

