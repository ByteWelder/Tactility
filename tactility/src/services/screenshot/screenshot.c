#include "screenshot.h"

#include "mutex.h"
#include "screenshot_task.h"
#include "service.h"
#include "service_registry.h"
#include "tactility_core.h"

#define TAG "sdcard_service"

typedef struct {
    Mutex* mutex;
    ScreenshotTask* task;
    ScreenshotMode mode;
} ServiceData;

static ServiceData* service_data_alloc() {
    ServiceData* data = malloc(sizeof(ServiceData));
    *data = (ServiceData) {
        .mutex = tt_mutex_alloc(MutexTypeNormal),
        .task = NULL
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

static void on_start(Service service) {
    ServiceData* data = service_data_alloc();
    tt_service_set_data(service, data);
}

static void on_stop(Service service) {
    ServiceData* data = tt_service_get_data(service);
    if (data->task) {
        screenshot_task_free(data->task);
        data->task = NULL;
    }
    tt_mutex_free(data->mutex);
    service_data_free(data);
}

const ServiceManifest screenshot_service = {
    .id = "screenshot",
    .on_start = &on_start,
    .on_stop = &on_stop
};

void tt_screenshot_start_apps(const char* path) {
    Service _Nullable service = tt_service_find(screenshot_service.id);
    if (service == NULL) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    ServiceData* data = tt_service_get_data(service);
    service_data_lock(data);
    if (data->task == NULL) {
        data->task = screenshot_task_alloc();
        data->mode = ScreenshotModeApps;
        screenshot_task_start_apps(data->task, path);
    } else {
        TT_LOG_E(TAG, "Screenshot task already running");
    }
    service_data_unlock(data);
}

void tt_screenshot_start_timed(const char* path, uint8_t delay_in_seconds, uint8_t amount) {
    Service _Nullable service = tt_service_find(screenshot_service.id);
    if (service == NULL) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    ServiceData* data = tt_service_get_data(service);
    service_data_lock(data);
    if (data->task == NULL) {
        data->task = screenshot_task_alloc();
        data->mode = ScreenshotModeTimed;
        screenshot_task_start_timed(data->task, path, delay_in_seconds, amount);
    } else {
        TT_LOG_E(TAG, "Screenshot task already running");
    }
    service_data_unlock(data);
}

void tt_screenshot_stop() {
    Service _Nullable service = tt_service_find(screenshot_service.id);
    if (service == NULL) {
        TT_LOG_E(TAG, "Service not found");
        return;
    }

    ServiceData* data = tt_service_get_data(service);
    service_data_lock(data);
    if (data->task != NULL) {
        screenshot_task_stop(data->task);
        screenshot_task_free(data->task);
        data->task = NULL;
        data->mode = ScreenshotModeNone;
    } else {
        TT_LOG_E(TAG, "Screenshot task not running");
    }
    service_data_unlock(data);
}

ScreenshotMode tt_screenshot_get_mode() {
    Service _Nullable service = tt_service_find(screenshot_service.id);
    if (service == NULL) {
        TT_LOG_E(TAG, "Service not found");
        return ScreenshotModeNone;
    } else {
        ServiceData* data = tt_service_get_data(service);
        service_data_lock(data);
        ScreenshotMode mode = data->mode;
        service_data_unlock(data);
        return mode;
    }
}

bool tt_screenshot_is_started() {
    return tt_screenshot_get_mode() != ScreenshotModeNone;
}
