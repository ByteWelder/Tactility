#include "screenshot_task.h"
#include "lv_screenshot.h"

#include "app.h"
#include "Mutex.h"
#include "services/loader/loader.h"
#include "tactility_core.h"
#include "thread.h"
#include "ui/lvgl_sync.h"

#define TAG "screenshot_task"

#define TASK_WORK_TYPE_DELAY 1
#define TASK_WORK_TYPE_APPS 2

#define SCREENSHOT_PATH_LIMIT 128

typedef struct {
    int type;
    uint8_t delay_in_seconds;
    uint8_t amount;
    char path[SCREENSHOT_PATH_LIMIT];
} ScreenshotTaskWork;

typedef struct {
    Thread* thread;
    Mutex* mutex;
    bool interrupted;
    ScreenshotTaskWork work;
} ScreenshotTaskData;

static void screenshot_task_lock(ScreenshotTaskData* data) {
    tt_check(tt_mutex_acquire(data->mutex, TtWaitForever) == TtStatusOk);
}

static void screenshot_task_unlock(ScreenshotTaskData* data) {
    tt_check(tt_mutex_release(data->mutex) == TtStatusOk);
}

ScreenshotTask* screenshot_task_alloc() {
    auto* data = static_cast<ScreenshotTaskData*>(malloc(sizeof(ScreenshotTaskData)));
    *data = (ScreenshotTaskData) {
        .thread = nullptr,
        .mutex = tt_mutex_alloc(MutexTypeRecursive),
        .interrupted = false
    };
    return data;
}

void screenshot_task_free(ScreenshotTask* task) {
    auto* data = static_cast<ScreenshotTaskData*>(task);
    if (data->thread) {
        screenshot_task_stop(data);
    }
}

static bool is_interrupted(ScreenshotTaskData* data) {
    screenshot_task_lock(data);
    bool interrupted = data->interrupted;
    screenshot_task_unlock(data);
    return interrupted;
}

static int32_t screenshot_task(void* context) {
    auto* data = static_cast<ScreenshotTaskData*>(context);

    bool interrupted = false;
    uint8_t screenshots_taken = 0;
    std::string last_app_id;

    while (!interrupted) {
        interrupted = is_interrupted(data);

        if (data->work.type == TASK_WORK_TYPE_DELAY) {
            // Splitting up the delays makes it easier to stop the service
            for (int i = 0; i < (data->work.delay_in_seconds * 10) && !is_interrupted(data); ++i){
                tt_delay_ms(100);
            }

            if (is_interrupted(data)) {
                break;
            }

            screenshots_taken++;
            char filename[SCREENSHOT_PATH_LIMIT + 32];
            sprintf(filename, "%s/screenshot-%d.png", data->work.path, screenshots_taken);
            tt_lvgl_lock(TtWaitForever);
            if (lv_screenshot_create(lv_scr_act(), LV_COLOR_FORMAT_NATIVE, LV_100ASK_SCREENSHOT_SV_PNG, filename)){
                TT_LOG_I(TAG, "Screenshot saved to %s", filename);
            } else {
                TT_LOG_E(TAG, "Screenshot not saved to %s", filename);
            }
            tt_lvgl_unlock();

            if (data->work.amount > 0 && screenshots_taken >= data->work.amount) {
                break; // Interrupted loop
            }
        } else if (data->work.type == TASK_WORK_TYPE_APPS) {
            App _Nullable app = loader_get_current_app();
            if (app) {
                const AppManifest* manifest = tt_app_get_manifest(app);
                if (manifest->id != last_app_id) {
                    tt_delay_ms(100);
                    last_app_id = manifest->id;

                    char filename[SCREENSHOT_PATH_LIMIT + 32];
                    sprintf(filename, "%s/screenshot-%s.png", data->work.path, manifest->id.c_str());
                    tt_lvgl_lock(TtWaitForever);
                    if (lv_screenshot_create(lv_scr_act(), LV_COLOR_FORMAT_NATIVE, LV_100ASK_SCREENSHOT_SV_PNG, filename)){
                        TT_LOG_I(TAG, "Screenshot saved to %s", filename);
                    } else {
                        TT_LOG_E(TAG, "Screenshot not saved to %s", filename);
                    }
                    tt_lvgl_unlock();
                }
            }
            tt_delay_ms(250);
        }
    }

    return 0;
}

static void screenshot_task_start(ScreenshotTaskData* data) {
    screenshot_task_lock(data);
    tt_check(data->thread == NULL);
    data->thread = tt_thread_alloc_ex(
        "screenshot",
        8192,
        &screenshot_task,
        data
    );
    tt_thread_start(data->thread);
    screenshot_task_unlock(data);
}

void screenshot_task_start_apps(ScreenshotTask* task, const char* path) {
    tt_check(strlen(path) < (SCREENSHOT_PATH_LIMIT - 1));
    auto* data = static_cast<ScreenshotTaskData*>(task);
    screenshot_task_lock(data);
    if (data->thread == nullptr) {
        data->interrupted = false;
        data->work.type = TASK_WORK_TYPE_APPS;
        strcpy(data->work.path, path);
        screenshot_task_start(data);
    } else {
        TT_LOG_E(TAG, "Task was already running");
    }
    screenshot_task_unlock(data);
}

void screenshot_task_start_timed(ScreenshotTask* task, const char* path, uint8_t delay_in_seconds, uint8_t amount) {
    tt_check(strlen(path) < (SCREENSHOT_PATH_LIMIT - 1));
    auto* data = static_cast<ScreenshotTaskData*>(task);
    screenshot_task_lock(data);
    if (data->thread == nullptr) {
        data->interrupted = false;
        data->work.type = TASK_WORK_TYPE_DELAY;
        data->work.delay_in_seconds = delay_in_seconds;
        data->work.amount = amount;
        strcpy(data->work.path, path);
        screenshot_task_start(data);
    } else {
        TT_LOG_E(TAG, "Task was already running");
    }
    screenshot_task_unlock(data);
}

void screenshot_task_stop(ScreenshotTask* task) {
    auto* data = static_cast<ScreenshotTaskData*>(task);
    if (data->thread != nullptr) {
        screenshot_task_lock(data);
        data->interrupted = true;
        screenshot_task_unlock(data);

        tt_thread_join(data->thread);

        screenshot_task_lock(data);
        tt_thread_free(data->thread);
        data->thread = nullptr;
        screenshot_task_unlock(data);
    }
}