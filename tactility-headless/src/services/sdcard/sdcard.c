#include <dirent.h>

#include "mutex.h"
#include "service.h"
#include "tactility_core.h"
#include "tactility_headless.h"

#define TAG "sdcard_service"

static int32_t sdcard_task(void* context);

typedef struct {
    Mutex* mutex;
    Thread* thread;
    SdcardState last_state;
    bool interrupted;
} ServiceData;

static ServiceData* service_data_alloc() {
    ServiceData* data = malloc(sizeof(ServiceData));
    *data = (ServiceData) {
        .mutex = tt_mutex_alloc(MutexTypeNormal),
        .thread = tt_thread_alloc_ex(
            "sdcard",
            3000, // Minimum is ~2800 @ ESP-IDF 5.1.2 when ejecting sdcard
            &sdcard_task,
            data
        ),
        .last_state = -1,
        .interrupted = false
    };
    tt_thread_set_priority(data->thread, ThreadPriorityLow);
    return data;
}

static void service_data_free(ServiceData* data) {
    tt_mutex_free(data->mutex);
    tt_thread_free(data->thread);
}

static void service_data_lock(ServiceData* data) {
    tt_check(tt_mutex_acquire(data->mutex, TtWaitForever) == TtStatusOk);
}

static void service_data_unlock(ServiceData* data) {
    tt_check(tt_mutex_release(data->mutex) == TtStatusOk);
}

static int32_t sdcard_task(void* context) {
    ServiceData* data = (ServiceData*)context;

    bool interrupted = false;

    do {
        service_data_lock(data);

        interrupted = data->interrupted;

        SdcardState new_state = tt_sdcard_get_state();

        if (new_state == SdcardStateError) {
            TT_LOG_W(TAG, "Sdcard error - unmounting. Did you eject the card in an unsafe manner?");
            tt_sdcard_unmount();
        }

        if (new_state != data->last_state) {
            data->last_state = new_state;
        }

        service_data_unlock(data);
        tt_delay_ms(2000);
    } while (!interrupted);

    return 0;
}

static void on_start(Service service) {
    if (tt_get_hardware_config()->sdcard != NULL) {
        ServiceData* data = service_data_alloc();
        tt_service_set_data(service, data);
        tt_thread_start(data->thread);
    } else {
        TT_LOG_I(TAG, "task not started due to config");
    }
}

static void on_stop(Service service) {
    ServiceData* data = tt_service_get_data(service);
    if (data != NULL) {
        service_data_lock(data);
        data->interrupted = true;
        service_data_unlock(data);

        tt_thread_join(data->thread);

        service_data_free(data);
    }
}

const ServiceManifest sdcard_service = {
    .id = "sdcard",
    .on_start = &on_start,
    .on_stop = &on_stop
};
