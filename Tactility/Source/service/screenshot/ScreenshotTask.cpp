#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#include <cstring>
#include "ScreenshotTask.h"
#include "lv_screenshot.h"

#include "app/AppContext.h"
#include "TactilityCore.h"
#include "service/loader/Loader.h"
#include "lvgl/LvglSync.h"

namespace tt::service::screenshot {

#define TAG "screenshot_task"

ScreenshotTask::~ScreenshotTask() {
    if (thread) {
        stop();
    }
}

bool ScreenshotTask::isInterrupted() {
    auto scoped_lockable = mutex.scoped();
    if (!scoped_lockable->lock(50 / portTICK_PERIOD_MS)) {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return true;
    }
    return interrupted;
}

bool ScreenshotTask::isFinished() {
    auto scoped_lockable = mutex.scoped();
    if (!scoped_lockable->lock(50 / portTICK_PERIOD_MS)) {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return false;
    }
    return finished;
}

void ScreenshotTask::setFinished() {
    auto scoped_lockable = mutex.scoped();
    scoped_lockable->lock();
    finished = true;
}

static void makeScreenshot(const char* filename) {
    if (lvgl::lock(50 / portTICK_PERIOD_MS)) {
        if (lv_screenshot_create(lv_scr_act(), LV_100ASK_SCREENSHOT_SV_PNG, filename)) {
            TT_LOG_I(TAG, "Screenshot saved to %s", filename);
        } else {
            TT_LOG_E(TAG, "Screenshot not saved to %s", filename);
        }
        lvgl::unlock();
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "LVGL");
    }
}

static int32_t screenshotTaskCallback(void* context) {
    auto* data = static_cast<ScreenshotTask*>(context);
    assert(data != nullptr);
    data->taskMain();
    return 0;
}

void ScreenshotTask::taskMain() {
    uint8_t screenshots_taken = 0;
    std::string last_app_id;

    while (!isInterrupted()) {
        if (work.type == TASK_WORK_TYPE_DELAY) {
            // Splitting up the delays makes it easier to stop the service
            for (int i = 0; i < (work.delay_in_seconds * 10) && !isInterrupted(); ++i){
                kernel::delayMillis(100);
            }

            if (!isInterrupted()) {
                screenshots_taken++;
                char filename[SCREENSHOT_PATH_LIMIT + 32];
                sprintf(filename, "%s/screenshot-%d.png", work.path, screenshots_taken);
                makeScreenshot(filename);

                if (work.amount > 0 && screenshots_taken >= work.amount) {
                    break; // Interrupted loop
                }
            }
        } else if (work.type == TASK_WORK_TYPE_APPS) {
            auto appContext = loader::getCurrentAppContext();
            if (appContext != nullptr) {
                const app::AppManifest& manifest = appContext->getManifest();
                if (manifest.id != last_app_id) {
                    kernel::delayMillis(100);
                    last_app_id = manifest.id;
                    char filename[SCREENSHOT_PATH_LIMIT + 32];
                    sprintf(filename, "%s/screenshot-%s.png", work.path, manifest.id.c_str());
                    makeScreenshot(filename);
                }
            }
            // Ensure the LVGL widgets are rendered as the app just started
            kernel::delayMillis(250);
        }
    }

    setFinished();
}

void ScreenshotTask::taskStart() {
    auto scoped_lockable = mutex.scoped();
    if (!scoped_lockable->lock(50 / portTICK_PERIOD_MS)) {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return;
    }

    tt_check(thread == nullptr);
    thread = new Thread(
        "screenshot",
        8192,
        &screenshotTaskCallback,
        this
    );
    thread->start();
}

void ScreenshotTask::startApps(const char* path) {
    tt_check(strlen(path) < (SCREENSHOT_PATH_LIMIT - 1));

    auto scoped_lockable = mutex.scoped();
    if (!scoped_lockable->lock(50 / portTICK_PERIOD_MS)) {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return;
    }

    if (thread == nullptr) {
        interrupted = false;
        work.type = TASK_WORK_TYPE_APPS;
        strcpy(work.path, path);
        taskStart();
    } else {
        TT_LOG_E(TAG, "Task was already running");
    }
}

void ScreenshotTask::startTimed(const char* path, uint8_t delay_in_seconds, uint8_t amount) {
    tt_check(strlen(path) < (SCREENSHOT_PATH_LIMIT - 1));
    auto scoped_lockable = mutex.scoped();
    if (!scoped_lockable->lock(50 / portTICK_PERIOD_MS)) {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return;
    }

    if (thread == nullptr) {
        interrupted = false;
        work.type = TASK_WORK_TYPE_DELAY;
        work.delay_in_seconds = delay_in_seconds;
        work.amount = amount;
        strcpy(work.path, path);
        taskStart();
    } else {
        TT_LOG_E(TAG, "Task was already running");
    }
}

void ScreenshotTask::stop() {
    if (thread != nullptr) {
        if (mutex.lock(50 / portTICK_PERIOD_MS)) {
            interrupted = true;
            tt_check(mutex.unlock());
        }

        thread->join();

        if (mutex.lock(50 / portTICK_PERIOD_MS)) {
            delete thread;
            thread = nullptr;
            tt_check(mutex.unlock());
        }
    }
}

} // namespace

#endif
