#include <Tactility/TactilityConfig.h>

#if TT_FEATURE_SCREENSHOT_ENABLED

#include "Tactility/service/screenshot/Screenshot.h"

#include <Tactility/service/ServiceContext.h>
#include <Tactility/service/ServiceRegistry.h>

#include <memory>

namespace tt::service::screenshot {

#define TAG "screenshot_service"

extern const ServiceManifest manifest;

std::shared_ptr<ScreenshotService> _Nullable optScreenshotService() {
    return service::findServiceById<ScreenshotService>(manifest.id);
}

void ScreenshotService::startApps(const std::string& path) {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(50 / portTICK_PERIOD_MS)) {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return;
    }

    if (task == nullptr || task->isFinished()) {
        task = std::make_unique<ScreenshotTask>();
        mode = Mode::Apps;
        task->startApps(path);
    } else {
        TT_LOG_W(TAG, "Screenshot task already running");
    }
}

void ScreenshotService::startTimed(const std::string& path, uint8_t delayInSeconds, uint8_t amount) {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(50 / portTICK_PERIOD_MS)) {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return;
    }

    if (task == nullptr || task->isFinished()) {
        task = std::make_unique<ScreenshotTask>();
        mode = Mode::Timed;
        task->startTimed(path, delayInSeconds, amount);
    } else {
        TT_LOG_W(TAG, "Screenshot task already running");
    }
}

void ScreenshotService::stop() {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(50 / portTICK_PERIOD_MS)) {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return;
    }

    if (task != nullptr) {
        task = nullptr;
        mode = Mode::None;
    } else {
        TT_LOG_W(TAG, "Screenshot task not running");
    }
}

Mode ScreenshotService::getMode() const {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(50 / portTICK_PERIOD_MS)) {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return Mode::None;
    }

    return mode;
}

bool ScreenshotService::isTaskStarted() {
    auto* current_task = task.get();
    if (current_task == nullptr) {
        return false;
    } else {
        return !current_task->isFinished();
    }
}

extern const ServiceManifest manifest = {
    .id = "Screenshot",
    .createService = create<ScreenshotService>
};

} // namespace

#endif