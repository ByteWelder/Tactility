#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#include <cstring>
#include "ScreenshotTask.h"
#include "lv_screenshot.h"

#include "app/AppContext.h"
#include "Mutex.h"
#include "TactilityCore.h"
#include "Thread.h"
#include "service/loader/Loader.h"
#include "lvgl/LvglSync.h"

namespace tt::service::screenshot::task {

#define TAG "screenshot_task"

#define TASK_WORK_TYPE_DELAY 1
#define TASK_WORK_TYPE_APPS 2

#define SCREENSHOT_PATH_LIMIT 128


struct ScreenshotTaskWork {
    int type = TASK_WORK_TYPE_DELAY ;
    uint8_t delay_in_seconds = 0;
    uint8_t amount = 0;
    char path[SCREENSHOT_PATH_LIMIT] = { 0 };
};

struct ScreenshotTaskData {
    Thread* thread = nullptr;
    Mutex mutex = Mutex(Mutex::TypeRecursive);
    bool interrupted = false;
    ScreenshotTaskWork work;
};

static void task_lock(ScreenshotTaskData* data) {
    tt_check(data->mutex.acquire(TtWaitForever) == TtStatusOk);
}

static void task_unlock(ScreenshotTaskData* data) {
    tt_check(data->mutex.release() == TtStatusOk);
}

ScreenshotTask* alloc() {
    return new ScreenshotTaskData();
}

void free(ScreenshotTask* task) {
    auto* data = static_cast<ScreenshotTaskData*>(task);
    if (data->thread) {
        stop(data);
    }
    delete data;
}

static bool is_interrupted(ScreenshotTaskData* data) {
    task_lock(data);
    bool interrupted = data->interrupted;
    task_unlock(data);
    return interrupted;
}

static void makeScreenshot(const char* filename) {
    if (lvgl::lock(50 / portTICK_PERIOD_MS)) {
#ifdef ESP_PLATFORM
        lv_color_format_t color_format = LV_COLOR_FORMAT_RGB888;
#else // Simulator
        lv_color_format_t color_format = LV_COLOR_FORMAT_NATIVE;
#endif
        if (lv_screenshot_create(lv_scr_act(), color_format, LV_100ASK_SCREENSHOT_SV_PNG, filename)) {
            TT_LOG_I(TAG, "Screenshot saved to %s", filename);
        } else {
            TT_LOG_E(TAG, "Screenshot not saved to %s", filename);
        }
        lvgl::unlock();
    } else {
        TT_LOG_E(TAG, "Failed to acquire LVGL lock");
    }
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
                kernel::delayMillis(100);
            }

            if (!is_interrupted(data)) {
                screenshots_taken++;
                char filename[SCREENSHOT_PATH_LIMIT + 32];
#ifdef ESP_PLATFORM
                sprintf(filename, "%s/%d.png", data->work.path, screenshots_taken);
#else
                sprintf(filename, "%s/screenshot-%d.png", data->work.path, screenshots_taken);
#endif
                makeScreenshot(filename);

                if (data->work.amount > 0 && screenshots_taken >= data->work.amount) {
                    break; // Interrupted loop
                }
            }
        } else if (data->work.type == TASK_WORK_TYPE_APPS) {
            app::AppContext* _Nullable app = loader::getCurrentApp();
            if (app) {
                const app::AppManifest& manifest = app->getManifest();
                if (manifest.id != last_app_id) {
                    kernel::delayMillis(100);
                    last_app_id = manifest.id;

                    char filename[SCREENSHOT_PATH_LIMIT + 32];
#ifdef ESP_PLATFORM
                    screenshots_taken++;
                    sprintf(filename, "%s/%d.png", data->work.path, screenshots_taken);
#else
                    sprintf(filename, "%s/screenshot-%s.png", data->work.path, manifest.id.c_str());
#endif
                    makeScreenshot(filename);

                }
            }
            // Ensure the LVGL widgets are rendered as the app just started
            kernel::delayMillis(250);
        }
    }

    return 0;
}

static void task_start(ScreenshotTaskData* data) {
    task_lock(data);
    tt_check(data->thread == nullptr);
    data->thread = new Thread(
        "screenshot",
        8192,
        &screenshot_task,
        data
    );
    data->thread->start();
    task_unlock(data);
}

void startApps(ScreenshotTask* task, const char* path) {
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

void startTimed(ScreenshotTask* task, const char* path, uint8_t delay_in_seconds, uint8_t amount) {
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

void stop(ScreenshotTask* task) {
    auto* data = static_cast<ScreenshotTaskData*>(task);
    if (data->thread != nullptr) {
        task_lock(data);
        data->interrupted = true;
        task_unlock(data);

        data->thread->join();

        task_lock(data);
        delete data->thread;
        data->thread = nullptr;
        task_unlock(data);
    }
}

} // namespace

#endif
