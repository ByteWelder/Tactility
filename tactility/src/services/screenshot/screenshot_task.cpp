#include "screenshot_task.h"
#include "lv_screenshot.h"

#include "app.h"
#include "Mutex.h"
#include "services/loader/loader.h"
#include "TactilityCore.h"
#include "Thread.h"
#include "ui/lvgl_sync.h"

namespace tt::service::screenshot {

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

static void task_lock(ScreenshotTaskData* data) {
    tt_check(tt_mutex_acquire(data->mutex, TtWaitForever) == TtStatusOk);
}

static void task_unlock(ScreenshotTaskData* data) {
    tt_check(tt_mutex_release(data->mutex) == TtStatusOk);
}

ScreenshotTask* task_alloc() {
    auto* data = static_cast<ScreenshotTaskData*>(malloc(sizeof(ScreenshotTaskData)));
    *data = (ScreenshotTaskData) {
        .thread = nullptr,
        .mutex = tt_mutex_alloc(MutexTypeRecursive),
        .interrupted = false
    };
    return data;
}

void task_free(ScreenshotTask* task) {
    auto* data = static_cast<ScreenshotTaskData*>(task);
    if (data->thread) {
        task_stop(data);
    }
}

static bool is_interrupted(ScreenshotTaskData* data) {
    task_lock(data);
    bool interrupted = data->interrupted;
    task_unlock(data);
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
                delay_ms(100);
            }

            if (is_interrupted(data)) {
                break;
            }

            screenshots_taken++;
            char filename[SCREENSHOT_PATH_LIMIT + 32];
            sprintf(filename, "%s/screenshot-%d.png", data->work.path, screenshots_taken);
            lvgl::lock(TtWaitForever);
            if (lv_screenshot_create(lv_scr_act(), LV_COLOR_FORMAT_NATIVE, LV_100ASK_SCREENSHOT_SV_PNG, filename)){
                TT_LOG_I(TAG, "Screenshot saved to %s", filename);
            } else {
                TT_LOG_E(TAG, "Screenshot not saved to %s", filename);
            }
            lvgl::unlock();

            if (data->work.amount > 0 && screenshots_taken >= data->work.amount) {
                break; // Interrupted loop
            }
        } else if (data->work.type == TASK_WORK_TYPE_APPS) {
            App _Nullable app = loader::get_current_app();
            if (app) {
                const AppManifest& manifest = tt_app_get_manifest(app);
                if (manifest.id != last_app_id) {
                    delay_ms(100);
                    last_app_id = manifest.id;

                    char filename[SCREENSHOT_PATH_LIMIT + 32];
                    sprintf(filename, "%s/screenshot-%s.png", data->work.path, manifest.id.c_str());
                    lvgl::lock(TtWaitForever);
                    if (lv_screenshot_create(lv_scr_act(), LV_COLOR_FORMAT_NATIVE, LV_100ASK_SCREENSHOT_SV_PNG, filename)){
                        TT_LOG_I(TAG, "Screenshot saved to %s", filename);
                    } else {
                        TT_LOG_E(TAG, "Screenshot not saved to %s", filename);
                    }
                    lvgl::unlock();
                }
            }
            delay_ms(250);
        }
    }

    return 0;
}

static void task_start(ScreenshotTaskData* data) {
    task_lock(data);
    tt_check(data->thread == NULL);
    data->thread = thread_alloc_ex(
        "screenshot",
        8192,
        &screenshot_task,
        data
    );
    thread_start(data->thread);
    task_unlock(data);
}

void task_start_apps(ScreenshotTask* task, const char* path) {
    tt_check(strlen(path) < (SCREENSHOT_PATH_LIMIT - 1));
    auto* data = static_cast<ScreenshotTaskData*>(task);
    task_lock(data);
    if (data->thread == nullptr) {
        data->interrupted = false;
        data->work.type = TASK_WORK_TYPE_APPS;
        strcpy(data->work.path, path);
        task_start(data);
    } else {
        TT_LOG_E(TAG, "Task was already running");
    }
    task_unlock(data);
}

void task_start_timed(ScreenshotTask* task, const char* path, uint8_t delay_in_seconds, uint8_t amount) {
    tt_check(strlen(path) < (SCREENSHOT_PATH_LIMIT - 1));
    auto* data = static_cast<ScreenshotTaskData*>(task);
    task_lock(data);
    if (data->thread == nullptr) {
        data->interrupted = false;
        data->work.type = TASK_WORK_TYPE_DELAY;
        data->work.delay_in_seconds = delay_in_seconds;
        data->work.amount = amount;
        strcpy(data->work.path, path);
        task_start(data);
    } else {
        TT_LOG_E(TAG, "Task was already running");
    }
    task_unlock(data);
}

void task_stop(ScreenshotTask* task) {
    auto* data = static_cast<ScreenshotTaskData*>(task);
    if (data->thread != nullptr) {
        task_lock(data);
        data->interrupted = true;
        task_unlock(data);

        thread_join(data->thread);

        task_lock(data);
        thread_free(data->thread);
        data->thread = nullptr;
        task_unlock(data);
    }
}

} // namespace
