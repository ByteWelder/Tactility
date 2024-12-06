#include "Assets.h"
#include "hal/Power.h"
#include "hal/sdcard/Sdcard.h"
#include "Mutex.h"
#include "service/ServiceContext.h"
#include "service/wifi/Wifi.h"
#include "Tactility.h"
#include "lvgl/Statusbar.h"
#include "service/ServiceRegistry.h"

namespace tt::service::statusbar {

#define TAG "statusbar_service"

extern const ServiceManifest manifest;

struct ServiceData {
    Mutex mutex;
    Thread thread;
    bool service_interrupted = false;
    int8_t wifi_icon_id = lvgl::statusbar_icon_add(nullptr);
    const char* wifi_last_icon = nullptr;
    int8_t sdcard_icon_id = lvgl::statusbar_icon_add(nullptr);
    const char* sdcard_last_icon = nullptr;
    int8_t power_icon_id = lvgl::statusbar_icon_add(nullptr);
    const char* power_last_icon = nullptr;

    ~ServiceData() {
        lvgl::statusbar_icon_remove(wifi_icon_id);
        lvgl::statusbar_icon_remove(sdcard_icon_id);
        lvgl::statusbar_icon_remove(power_icon_id);
    }

    void lock() const {
        tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    }

    void unlock() const {
        tt_check(mutex.release() == TtStatusOk);
    }
};

// region wifi

const char* getWifiStatusIconForRssi(int rssi, bool secured) {
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
            rssi = wifi::getRssi();
            return getWifiStatusIconForRssi(rssi, secure);
        default:
            tt_crash("not implemented");
    }
}

static void update_wifi_icon(std::shared_ptr<ServiceData> data) {
    wifi::WifiRadioState radio_state = wifi::getRadioState();
    bool is_secure = wifi::isConnectionSecure();
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

static void update_sdcard_icon(std::shared_ptr<ServiceData> data) {
    hal::sdcard::State state = hal::sdcard::getState();
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
        uint8_t charge = power->getChargeLevel();
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

static void update_power_icon(std::shared_ptr<ServiceData> data) {
    const char* desired_icon = power_get_status_icon();
    if (data->power_last_icon != desired_icon) {
        lvgl::statusbar_icon_set_image(data->power_icon_id, desired_icon);
        lvgl::statusbar_icon_set_visibility(data->power_icon_id, desired_icon != nullptr);
        data->power_last_icon = desired_icon;
    }
}

// endregion power

// region service

static void service_data_free(ServiceData* data) {
   free(data);
}

int32_t serviceMain(TT_UNUSED void* parameter) {
    TT_LOG_I(TAG, "Started main loop");
    delay_ms(20); // TODO: Make service instance findable earlier on (but expose "starting" state?)
    auto context = tt::service::findServiceById(manifest.id);
    if (context == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return -1;
    }

    auto data = std::static_pointer_cast<ServiceData>(context->getData());

    while (!data->service_interrupted) {
        update_wifi_icon(data);
        update_sdcard_icon(data);
        update_power_icon(data);
        delay_ms(1000);
    }
    return 0;
}

static void onStart(ServiceContext& service) {
    auto data = std::make_shared<ServiceData>();
    service.setData(data);

    lvgl::statusbar_icon_set_visibility(data->wifi_icon_id, true);
    update_wifi_icon(data);
    update_sdcard_icon(data); // also updates visibility
    update_power_icon(data);


    data->thread.setCallback(serviceMain, nullptr);
    data->thread.setPriority(Thread::PriorityLow);
    data->thread.setStackSize(3000);
    data->thread.setName("statusbar");
    data->thread.start();
}

static void onStop(ServiceContext& service) {
    auto data = std::static_pointer_cast<ServiceData>(service.getData());

    // Stop thread
    data->lock();
    data->service_interrupted = true;
    data->unlock();
    data->thread.join();
}

extern const ServiceManifest manifest = {
    .id = "Statusbar",
    .onStart = onStart,
    .onStop = onStop
};

// endregion service

} // namespace
