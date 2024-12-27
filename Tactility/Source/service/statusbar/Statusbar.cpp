#include "Assets.h"
#include "Mutex.h"
#include "Timer.h"
#include "Tactility.h"

#include "hal/Power.h"
#include "hal/SdCard.h"
#include "lvgl/Statusbar.h"
#include "service/ServiceContext.h"
#include "service/wifi/Wifi.h"
#include "service/ServiceRegistry.h"
#include "TactilityHeadless.h"

namespace tt::service::statusbar {

#define TAG "statusbar_service"

extern const ServiceManifest manifest;

struct ServiceData {
    Mutex mutex;
    std::unique_ptr<Timer> updateTimer;
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

const char* getWifiStatusIconForRssi(int rssi) {
    if (rssi >= -60) {
        return TT_ASSETS_ICON_WIFI_SIGNAL_STRONG_WHITE;
    } else if (rssi >= -70) {
        return TT_ASSETS_ICON_WIFI_SIGNAL_MEDIUM_WHITE;
    } else {
        return TT_ASSETS_ICON_WIFI_SIGNAL_WEAK_WHITE;
    }
}

static const char* wifi_get_status_icon(wifi::WifiRadioState state, bool secure) {
    int rssi;
    switch (state) {
        case wifi::WIFI_RADIO_ON:
        case wifi::WIFI_RADIO_ON_PENDING:
        case wifi::WIFI_RADIO_CONNECTION_PENDING:
            return TT_ASSETS_ICON_WIFI_SCAN_WHITE;
        case wifi::WIFI_RADIO_OFF_PENDING:
        case wifi::WIFI_RADIO_OFF:
            return TT_ASSETS_ICON_WIFI_OFF_WHITE;
        case wifi::WIFI_RADIO_CONNECTION_ACTIVE:
            rssi = wifi::getRssi();
            return getWifiStatusIconForRssi(rssi);
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

static const char* sdcard_get_status_icon(hal::SdCard::State state) {
    switch (state) {
        case hal::SdCard::StateMounted:
            return TT_ASSETS_ICON_SDCARD;
        case hal::SdCard::StateError:
        case hal::SdCard::StateUnmounted:
        case hal::SdCard::StateUnknown:
            return TT_ASSETS_ICON_SDCARD_ALERT;
        default:
            tt_crash("Unhandled SdCard state");
    }
}

static void update_sdcard_icon(std::shared_ptr<ServiceData> data) {
    auto sdcard = tt::hal::getConfiguration().sdcard;
    if (sdcard != nullptr) {
        auto state = sdcard->getState();
        const char* desired_icon = sdcard_get_status_icon(state);
        if (data->sdcard_last_icon != desired_icon) {
            lvgl::statusbar_icon_set_image(data->sdcard_icon_id, desired_icon);
            lvgl::statusbar_icon_set_visibility(data->sdcard_icon_id, desired_icon != nullptr);
            data->sdcard_last_icon = desired_icon;
        }
    }
}

// endregion sdcard

// region power

static _Nullable const char* power_get_status_icon() {
    auto get_power = getConfiguration()->hardware->power;
    if (get_power == nullptr) {
        return nullptr;
    }

    auto power = get_power();

    hal::Power::MetricData charge_level;
    if (!power->getMetric(hal::Power::MetricType::CHARGE_LEVEL, charge_level)) {
        return nullptr;
    }

    uint8_t charge = charge_level.valueAsUint8;

    if (charge >= 95) {
        return TT_ASSETS_ICON_POWER_100;
    } else if (charge >= 85) {
        return TT_ASSETS_ICON_POWER_90;
    } else if (charge >= 75) {
        return TT_ASSETS_ICON_POWER_80;
    } else if (charge >= 65) {
        return TT_ASSETS_ICON_POWER_70;
    } else if (charge >= 55) {
        return TT_ASSETS_ICON_POWER_60;
    } else if (charge >= 45) {
        return TT_ASSETS_ICON_POWER_50;
    } else if (charge >= 35) {
        return TT_ASSETS_ICON_POWER_40;
    } else if (charge >= 25) {
        return TT_ASSETS_ICON_POWER_30;
    } else if (charge >= 15) {
        return TT_ASSETS_ICON_POWER_20;
    } else if (charge >= 5) {
        return TT_ASSETS_ICON_POWER_10;
    } else  {
        return TT_ASSETS_ICON_POWER_0;
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

static void onUpdate(std::shared_ptr<void> parameter) {
    auto data = std::static_pointer_cast<ServiceData>(parameter);
    // TODO: Make thread-safe for LVGL
    update_wifi_icon(data);
    update_sdcard_icon(data);
    update_power_icon(data);
}

static void onStart(ServiceContext& service) {
    auto data = std::make_shared<ServiceData>();
    service.setData(data);

    // TODO: Make thread-safe for LVGL
    lvgl::statusbar_icon_set_visibility(data->wifi_icon_id, true);
    update_wifi_icon(data);
    update_sdcard_icon(data); // also updates visibility
    update_power_icon(data);

    data->updateTimer = std::make_unique<Timer>(Timer::TypePeriodic, onUpdate, data);
    // We want to try and scan more often in case of startup or scan lock failure
    data->updateTimer->start(1000);
}

static void onStop(ServiceContext& service) {
    auto data = std::static_pointer_cast<ServiceData>(service.getData());

    // Stop thread
    data->updateTimer->stop();
    data->updateTimer = nullptr;
}

extern const ServiceManifest manifest = {
    .id = "Statusbar",
    .onStart = onStart,
    .onStop = onStop
};

// endregion service

} // namespace
