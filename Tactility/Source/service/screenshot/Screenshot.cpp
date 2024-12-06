#include "Screenshot.h"
#include <cstdlib>
#include <memory>

#include "Mutex.h"
#include "ScreenshotTask.h"
#include "service/ServiceContext.h"
#include "service/ServiceRegistry.h"
#include "TactilityCore.h"

namespace tt::service::screenshot {

#define TAG "screenshot_service"

extern const ServiceManifest manifest;

struct ServiceData {
    Mutex mutex;
    task::ScreenshotTask* task = nullptr;
    Mode mode = ScreenshotModeNone;

    ~ServiceData() {
        if (task) {
            task::free(task);
        }
    }

    void lock() {
        tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    }

    void unlock() {
        tt_check(mutex.release() == TtStatusOk);
    }
};

void startApps(const char* path) {
    _Nullable auto* service = findServiceById(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    auto data = std::static_pointer_cast<ServiceData>(service->getData());
    data->lock();
    if (data->task == nullptr) {
        data->task = task::alloc();
        data->mode = ScreenshotModeApps;
        task::startApps(data->task, path);
    } else {
        TT_LOG_E(TAG, "Screenshot task already running");
    }
    data->unlock();
}

void startTimed(const char* path, uint8_t delay_in_seconds, uint8_t amount) {
    _Nullable auto* service = findServiceById(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    auto data = std::static_pointer_cast<ServiceData>(service->getData());
    data->lock();
    if (data->task == nullptr) {
        data->task = task::alloc();
        data->mode = ScreenshotModeTimed;
        task::startTimed(data->task, path, delay_in_seconds, amount);
    } else {
        TT_LOG_E(TAG, "Screenshot task already running");
    }
    data->unlock();
}

void stop() {
    _Nullable ServiceContext* service = findServiceById(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    auto data = std::static_pointer_cast<ServiceData>(service->getData());
    data->lock();
    if (data->task != nullptr) {
        task::stop(data->task);
        task::free(data->task);
        data->task = nullptr;
        data->mode = ScreenshotModeNone;
    } else {
        TT_LOG_E(TAG, "Screenshot task not running");
    }
    data->unlock();
}

Mode getMode() {
    _Nullable auto* service = findServiceById(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return ScreenshotModeNone;
    } else {
        auto data = std::static_pointer_cast<ServiceData>(service->getData());
        data->lock();
        Mode mode = data->mode;
        data->unlock();
        return mode;
    }
}

bool isStarted() {
    return getMode() != ScreenshotModeNone;
}

static void onStart(ServiceContext& service) {
    auto data = std::make_shared<ServiceData>();
    service.setData(data);
}

extern const ServiceManifest manifest = {
    .id = "Screenshot",
    .onStart = onStart
};

} // namespace
