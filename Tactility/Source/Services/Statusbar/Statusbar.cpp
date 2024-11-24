#include "Assets.h"
#include "Hal/Power.h"
#include "Hal/Sdcard.h"
#include "Mutex.h"
#include "Service.h"
#include "Services/Wifi/Wifi.h"
#include "Tactility.h"
#include "Ui/Statusbar.h"

namespace tt::service::statusbar {

#define TAG "statusbar_service"

typedef struct {
    Mutex* mutex;
    Thread* thread;
    bool service_interrupted;
    int8_t wifi_icon_id;
    const char* wifi_last_icon;
    int8_t sdcard_icon_id;
    const char* sdcard_last_icon;
    int8_t power_icon_id;
    const char* power_last_icon;
} ServiceData;

// region wifi

const char* get_status_icon_for_rssi(int rssi, bool secured) {
    if (rssi > 0) {
        return TT_ASSETS_ICON_WIFI_CONNECTION_ISSUE;
    } else if (rssi >= -30) {
        return secured ? TT_ASSETS_ICON_WIFI_SIGNAL_4_LOCKED : TT_ASSETS_ICON_WIFI_SIGNAL_4;
    } else if (rssi >= -67) {
        return secured ? TT_ASSETS_ICON_WIFI_SIGNAL_3_LOCKED : TT_ASSETS_ICON_WIFI_SIGNAL_3;
    } else if (rssi >= -70) {
        return secured ? TT_ASSETS_ICON_WIFI_SIGNAL_2_LOCKED : TT_ASSETS_ICON_WIFI_SIGNAL_2;
    } else if (rssi >= -80) {
        return secured ? TT_ASSETS_ICON_WIFI_SIGNAL_1_LOCKED : TT_ASSETS_ICON_WIFI_SIGNAL_1;
    } else {
        return secured ? TT_ASSETS_ICON_WIFI_SIGNAL_0_LOCKED : TT_ASSETS_ICON_WIFI_SIGNAL_0;
    }
}

static const char* wifi_get_status_icon(wifi::WifiRadioState state, bool secure) {
    int rssi;
    switch (state) {
        case wifi::WIFI_RADIO_ON_PENDING:
        case wifi::WIFI_RADIO_ON:
        case wifi::WIFI_RADIO_OFF_PENDING:
        case wifi::WIFI_RADIO_OFF:
            return TT_ASSETS_ICON_WIFI_OFF;
        case wifi::WIFI_RADIO_CONNECTION_PENDING:
            return TT_ASSETS_ICON_WIFI_FIND;
        case wifi::WIFI_RADIO_CONNECTION_ACTIVE:
            rssi = wifi::get_rssi();
            return get_status_icon_for_rssi(rssi, secure);
        default:
            tt_crash("not implemented");
    }
}

static void update_wifi_icon(ServiceData* data) {
    wifi::WifiRadioState radio_state = wifi::get_radio_state();
    bool is_secure = wifi::is_connection_secure();
    const char* desired_icon = wifi_get_status_icon(radio_state, is_secure);
    if (data->wifi_last_icon != desired_icon) {
        lvgl::statusbar_icon_set_image(data->wifi_icon_id, desired_icon);
        data->wifi_last_icon = desired_icon;
    }
}

// endregion wifi

// region sdcard

static _Nullable const char* sdcard_get_status_icon(hal::sdcard::State state) {
    switch (state) {
        case hal::sdcard::StateMounted:
            return TT_ASSETS_ICON_SDCARD;
        case hal::sdcard::StateError:
        case hal::sdcard::StateUnmounted:
            return TT_ASSETS_ICON_SDCARD_ALERT;
        default:
            return nullptr;
    }
}

static void update_sdcard_icon(ServiceData* data) {
    hal::sdcard::State state = hal::sdcard::get_state();
    const char* desired_icon = sdcard_get_status_icon(state);
    if (data->sdcard_last_icon != desired_icon) {
        lvgl::statusbar_icon_set_image(data->sdcard_icon_id, desired_icon);
        lvgl::statusbar_icon_set_visibility(data->sdcard_icon_id, desired_icon != nullptr);
        data->sdcard_last_icon = desired_icon;
    }
}

// endregion sdcard

// region power

static _Nullable const char* power_get_status_icon() {
    _Nullable const hal::Power* power = getConfiguration()->hardware->power;
    if (power != nullptr) {
        uint8_t charge = power->get_charge_level();
        if (charge >= 230) {
            return TT_ASSETS_ICON_POWER_100;
        } else if (charge >= 161) {
            return TT_ASSETS_ICON_POWER_080;
        } else if (charge >= 127) {
            return TT_ASSETS_ICON_POWER_060;
        } else if (charge >= 76) {
            return TT_ASSETS_ICON_POWER_040;
        } else {
            return TT_ASSETS_ICON_POWER_020;
        }
    } else {
        return nullptr;
    }
}

static void update_power_icon(ServiceData* data) {
    const char* desired_icon = power_get_status_icon();
    if (data->power_last_icon != desired_icon) {
        lvgl::statusbar_icon_set_image(data->power_icon_id, desired_icon);
        lvgl::statusbar_icon_set_visibility(data->power_icon_id, desired_icon != nullptr);
        data->power_last_icon = desired_icon;
    }
}

// endregion power

// region service

static ServiceData* service_data_alloc() {
    auto* data = static_cast<ServiceData*>(malloc(sizeof(ServiceData)));
    *data = (ServiceData) {
        .mutex = tt_mutex_alloc(MutexTypeNormal),
        .thread = new Thread(),
        .service_interrupted = false,
        .wifi_icon_id = lvgl::statusbar_icon_add(nullptr),
        .wifi_last_icon = nullptr,
        .sdcard_icon_id = lvgl::statusbar_icon_add(nullptr),
        .sdcard_last_icon = nullptr,
        .power_icon_id = lvgl::statusbar_icon_add(nullptr),
        .power_last_icon = nullptr
    };

    lvgl::statusbar_icon_set_visibility(data->wifi_icon_id, true);
    update_wifi_icon(data);
    update_sdcard_icon(data); // also updates visibility
    update_power_icon(data);

    return data;
}

static void service_data_free(ServiceData* data) {
    tt_mutex_free(data->mutex);
    delete data->thread;
    lvgl::statusbar_icon_remove(data->wifi_icon_id);
    lvgl::statusbar_icon_remove(data->sdcard_icon_id);
    lvgl::statusbar_icon_remove(data->power_icon_id);
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
    auto* data = (ServiceData*)parameter;
    tt_assert(data != nullptr);
    while (!data->service_interrupted) {
        update_wifi_icon(data);
        update_sdcard_icon(data);
        update_power_icon(data);
        delay_ms(1000);
    }
    return 0;
}

static void on_start(Service& service) {
    ServiceData* data = service_data_alloc();
    service.setData(data);

    data->thread->setCallback(service_main, data);
    data->thread->setPriority(Thread::PriorityLow);
    data->thread->setStackSize(3000);
    data->thread->start();
}

static void on_stop(Service& service) {
    auto* data = static_cast<ServiceData*>(service.getData());

    // Stop thread
    service_data_lock(data);
    data->service_interrupted = true;
    service_data_unlock(data);
    tt_mutex_release(data->mutex);
    data->thread->join();

    service_data_free(data);
}

extern const ServiceManifest manifest = {
    .id = "Statusbar",
    .on_start = &on_start,
    .on_stop = &on_stop
};

// endregion service

} // namespace
