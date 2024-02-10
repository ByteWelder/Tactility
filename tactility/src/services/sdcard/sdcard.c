#include <dirent.h>

#include "assets.h"
#include "mutex.h"
#include "service.h"
#include "tactility.h"
#include "tactility_core.h"
#include "ui/statusbar.h"

#define TAG "sdcard_service"

static int32_t sdcard_task(TT_UNUSED void* context);

typedef struct {
    Mutex* mutex;
    Thread* thread;
    SdcardState last_state;
    int8_t statusbar_icon_id;
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
        .statusbar_icon_id = tt_statusbar_icon_add(NULL),
        .interrupted = false
    };
    return data;
}

static void service_data_free(ServiceData* data) {
    tt_mutex_free(data->mutex);
    tt_statusbar_icon_remove(data->statusbar_icon_id);
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

    // We set NULL as statusbar image by default, so it's hidden by default
    tt_statusbar_icon_set_visibility(data->statusbar_icon_id, true);

    do {
        service_data_lock(data);

        interrupted = data->interrupted;

        SdcardState new_state = tt_sdcard_get_state();

        if (new_state == SdcardStateError) {
            TT_LOG_W(TAG, "Sdcard error - unmounting. Did you eject the card in an unsafe manner?");
            tt_sdcard_unmount();
        }

        if (new_state != data->last_state) {
            TT_LOG_I(TAG, "State change %d -> %d", data->last_state, new_state);
            if (new_state == SdcardStateMounted) {
                tt_statusbar_icon_set_image(data->statusbar_icon_id, TT_ASSETS_ICON_SDCARD);
            } else {
                tt_statusbar_icon_set_image(data->statusbar_icon_id, TT_ASSETS_ICON_SDCARD_ALERT);
            }
            data->last_state = new_state;
        }

        service_data_unlock(data);
        tt_delay_ms(2000);
    } while (!interrupted);

    return 0;
}

static void on_start(Service service) {
    if (tt_get_config()->hardware->sdcard != NULL) {
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
