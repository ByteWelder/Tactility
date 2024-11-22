#include "Screenshot.h"
#include <cstdlib>

#include "Mutex.h"
#include "ScreenshotTask.h"
#include "Service.h"
#include "ServiceRegistry.h"
#include "TactilityCore.h"

namespace tt::service::screenshot {

#define TAG "sdcard_service"

extern const ServiceManifest manifest;

typedef struct {
    Mutex* mutex;
    ScreenshotTask* task;
    ScreenshotMode mode;
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

static void on_start(Service& service) {
    ServiceData* data = service_data_alloc();
    service.setData(data);
}

static void on_stop(Service& service) {
    auto* data = static_cast<ServiceData*>(service.getData());
    if (data->task) {
        task_free(data->task);
        data->task = nullptr;
    }
    tt_mutex_free(data->mutex);
    service_data_free(data);
}

void start_apps(const char* path) {
    _Nullable auto* service = service_find(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    auto* data = static_cast<ServiceData*>(service->getData());
    service_data_lock(data);
    if (data->task == nullptr) {
        data->task = task_alloc();
        data->mode = ScreenshotModeApps;
        task_start_apps(data->task, path);
    } else {
        TT_LOG_E(TAG, "Screenshot task already running");
    }
    service_data_unlock(data);
}

void start_timed(const char* path, uint8_t delay_in_seconds, uint8_t amount) {
    _Nullable auto* service = service_find(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    auto* data = static_cast<ServiceData*>(service->getData());
    service_data_lock(data);
    if (data->task == nullptr) {
        data->task = task_alloc();
        data->mode = ScreenshotModeTimed;
        task_start_timed(data->task, path, delay_in_seconds, amount);
    } else {
        TT_LOG_E(TAG, "Screenshot task already running");
    }
    service_data_unlock(data);
}

void stop() {
    _Nullable Service* service = service_find(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    auto data = static_cast<ServiceData*>(service->getData());
    service_data_lock(data);
    if (data->task != nullptr) {
        task_stop(data->task);
        task_free(data->task);
        data->task = nullptr;
        data->mode = ScreenshotModeNone;
    } else {
        TT_LOG_E(TAG, "Screenshot task not running");
    }
    service_data_unlock(data);
}

ScreenshotMode get_mode() {
    _Nullable auto* service = service_find(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return ScreenshotModeNone;
    } else {
        auto* data = static_cast<ServiceData*>(service->getData());
        service_data_lock(data);
        ScreenshotMode mode = data->mode;
        service_data_unlock(data);
        return mode;
    }
}

bool is_started() {
    return get_mode() != ScreenshotModeNone;
}

extern const ServiceManifest manifest = {
    .id = "screenshot",
    .on_start = &on_start,
    .on_stop = &on_stop
};

} // namespace
