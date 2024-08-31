#include "mutex.h"
#include "service.h"
#include "ui/statusbar.h"

#define TAG "wifi_statusbar_service"

#include "assets.h"
#include "sdcard.h"
#include "services/wifi/wifi.h"

typedef struct {
    Mutex* mutex;
    Thread* thread;
    bool service_interrupted;
    int8_t wifi_icon_id;
    const char* wifi_last_icon;
    int8_t sdcard_icon_id;
    const char* sdcard_last_icon;
} ServiceData;

const char* wifi_get_status_icon_for_rssi(int rssi, bool secured) {
    if (rssi > 0) {
        return TT_ASSETS_ICON_WIFI_CONNECTION_ISSUE;
    } else if (rssi > -67) {
        return secured ? TT_ASSETS_ICON_WIFI_SIGNAL_4_LOCKED : TT_ASSETS_ICON_WIFI_SIGNAL_4;
    } else if (rssi > -70) {
        return secured ? TT_ASSETS_ICON_WIFI_SIGNAL_3_LOCKED : TT_ASSETS_ICON_WIFI_SIGNAL_3;
    } else if (rssi > -80) {
        return secured ? TT_ASSETS_ICON_WIFI_SIGNAL_2_LOCKED : TT_ASSETS_ICON_WIFI_SIGNAL_2;
    } else {
        return secured ? TT_ASSETS_ICON_WIFI_SIGNAL_1_LOCKED : TT_ASSETS_ICON_WIFI_SIGNAL_1;
    }
}

static const char* wifi_get_status_icon(WifiRadioState state, bool secure) {
    int rssi;
    switch (state) {
        case WIFI_RADIO_ON_PENDING:
        case WIFI_RADIO_ON:
        case WIFI_RADIO_OFF_PENDING:
        case WIFI_RADIO_OFF:
            return TT_ASSETS_ICON_WIFI_OFF;
        case WIFI_RADIO_CONNECTION_PENDING:
            return TT_ASSETS_ICON_WIFI_FIND;
        case WIFI_RADIO_CONNECTION_ACTIVE:
            rssi = wifi_get_rssi();
            return wifi_get_status_icon_for_rssi(rssi, secure);
        default:
            tt_crash_implementation("not implemented");
    }
}

static _Nullable const char* sdcard_get_status_icon(SdcardState state) {
    switch (state) {
        case SdcardStateMounted:
            return TT_ASSETS_ICON_SDCARD;
        case SdcardStateError:
        case SdcardStateUnmounted:
            return TT_ASSETS_ICON_SDCARD_ALERT;
        default:
            return NULL;
    }
}

static void update_wifi_icon(ServiceData* data) {
    WifiRadioState radio_state = wifi_get_radio_state();
    bool is_secure = wifi_is_connection_secure();
    const char* desired_icon = wifi_get_status_icon(radio_state, is_secure);
    if (data->wifi_last_icon != desired_icon) {
        tt_statusbar_icon_set_image(data->wifi_icon_id, desired_icon);
        data->wifi_last_icon = desired_icon;
    }
}

static void update_sdcard_icon(ServiceData* data) {
    SdcardState state = tt_sdcard_get_state();
    const char* desired_icon = sdcard_get_status_icon(state);
    if (data->sdcard_last_icon != desired_icon) {
        tt_statusbar_icon_set_image(data->sdcard_icon_id, desired_icon);
        tt_statusbar_icon_set_visibility(data->sdcard_icon_id, desired_icon != NULL);
        data->sdcard_last_icon = desired_icon;
    }
}

static ServiceData* service_data_alloc() {
    ServiceData* data = malloc(sizeof(ServiceData));
    *data = (ServiceData) {
        .mutex = tt_mutex_alloc(MutexTypeNormal),
        .thread = tt_thread_alloc(),
        .service_interrupted = false,
        .wifi_icon_id = tt_statusbar_icon_add(NULL),
        .wifi_last_icon = NULL,
        .sdcard_icon_id = tt_statusbar_icon_add(NULL),
        .sdcard_last_icon = NULL,
    };

    tt_statusbar_icon_set_visibility(data->wifi_icon_id, true);
    update_wifi_icon(data);
    update_sdcard_icon(data); // also updates visibility

    return data;
}

static void service_data_free(ServiceData* data) {
    tt_mutex_free(data->mutex);
    tt_thread_free(data->thread);
    tt_statusbar_icon_remove(data->wifi_icon_id);
    free(data);
}

static void service_data_lock(ServiceData* data) {
    tt_check(tt_mutex_acquire(data->mutex, TtWaitForever) == TtStatusOk);
}

static void service_data_unlock(ServiceData* data) {
    tt_check(tt_mutex_release(data->mutex) == TtStatusOk);
}

int32_t service_main(TT_UNUSED void* parameter) {
    TT_LOG_I(TAG, "Started main loop");
    ServiceData* data = (ServiceData*)parameter;
    tt_check(data != NULL);
    while (!data->service_interrupted) {
        update_wifi_icon(data);
        update_sdcard_icon(data);
        tt_delay_ms(1000);
    }
    return 0;
}

static void on_start(Service service) {
    ServiceData* data = service_data_alloc();

    tt_service_set_data(service, data);

    tt_thread_set_callback(data->thread, service_main);
    tt_thread_set_current_priority(ThreadPriorityLow);
    tt_thread_set_stack_size(data->thread, 2048); // 2048 was the minimum when last tested
    tt_thread_set_context(data->thread, data);
    tt_thread_start(data->thread);
}

static void on_stop(Service service) {
    ServiceData* data = tt_service_get_data(service);

    // Stop thread
    service_data_lock(data);
    data->service_interrupted = true;
    service_data_unlock(data);
    tt_mutex_release(data->mutex);
    tt_thread_join(data->thread);

    service_data_free(data);
}

const ServiceManifest statusbar_updater_service = {
    .id = "statusbar_updater",
    .on_start = &on_start,
    .on_stop = &on_stop
};
