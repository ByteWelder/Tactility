#include "Screenshot.h"
#include <cstdlib>

#include "Mutex.h"
#include "ScreenshotTask.h"
#include "service/ServiceContext.h"
#include "service/ServiceRegistry.h"
#include "TactilityCore.h"

namespace tt::service::screenshot {

#define TAG "screenshot_service"

extern const ServiceManifest manifest;

typedef struct {
    Mutex* mutex;
    task::ScreenshotTask* task;
    Mode mode;
} ServiceData;

static ServiceData* service_data_alloc() {
    auto* data = static_cast<ServiceData*>(malloc(sizeof(ServiceData)));
    *data = (ServiceData) {
        .mutex = tt_mutex_alloc(MutexTypeNormal),
        .task = nullptr,
        .mode = ScreenshotModeNone
    };
    return data;
}

static void service_data_free(ServiceData* data) {
    tt_mutex_free(data->mutex);
}

static void service_data_lock(ServiceData* data) {
    tt_check(tt_mutex_acquire(data->mutex, TtWaitForever) == TtStatusOk);
}

static void service_data_unlock(ServiceData* data) {
    tt_check(tt_mutex_release(data->mutex) == TtStatusOk);
}

static void on_start(ServiceContext& service) {
    ServiceData* data = service_data_alloc();
    service.setData(data);
}

static void on_stop(ServiceContext& service) {
    auto* data = static_cast<ServiceData*>(service.getData());
    if (data->task) {
        task::free(data->task);
        data->task = nullptr;
    }
    tt_mutex_free(data->mutex);
    service_data_free(data);
}

void startApps(const char* path) {
    _Nullable auto* service = findServiceById(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    auto* data = static_cast<ServiceData*>(service->getData());
    service_data_lock(data);
    if (data->task == nullptr) {
        data->task = task::alloc();
        data->mode = ScreenshotModeApps;
        task::startApps(data->task, path);
    } else {
        TT_LOG_E(TAG, "Screenshot task already running");
    }
    service_data_unlock(data);
}

void startTimed(const char* path, uint8_t delay_in_seconds, uint8_t amount) {
    _Nullable auto* service = findServiceById(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    auto* data = static_cast<ServiceData*>(service->getData());
    service_data_lock(data);
    if (data->task == nullptr) {
        data->task = task::alloc();
        data->mode = ScreenshotModeTimed;
        task::startTimed(data->task, path, delay_in_seconds, amount);
    } else {
        TT_LOG_E(TAG, "Screenshot task already running");
    }
    service_data_unlock(data);
}

void stop() {
    _Nullable ServiceContext* service = findServiceById(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    auto data = static_cast<ServiceData*>(service->getData());
    service_data_lock(data);
    if (data->task != nullptr) {
        task::stop(data->task);
        task::free(data->task);
        data->task = nullptr;
        data->mode = ScreenshotModeNone;
    } else {
        TT_LOG_E(TAG, "Screenshot task not running");
    }
    service_data_unlock(data);
}

Mode getMode() {
    _Nullable auto* service = findServiceById(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return ScreenshotModeNone;
    } else {
        auto* data = static_cast<ServiceData*>(service->getData());
        service_data_lock(data);
        Mode mode = data->mode;
        service_data_unlock(data);
        return mode;
    }
}

bool isStarted() {
    return getMode() != ScreenshotModeNone;
}

extern const ServiceManifest manifest = {
    .id = "Screenshot",
    .onStart = &on_start,
    .onStop = &on_stop
};

} // namespace
