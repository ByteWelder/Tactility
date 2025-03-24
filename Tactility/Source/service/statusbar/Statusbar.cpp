#include "Tactility/lvgl/Statusbar.h"
#include "Tactility/lvgl/LvglSync.h"

#include "Tactility/hal/power/PowerDevice.h"
#include "Tactility/hal/sdcard/SdCardDevice.h"
#include "Tactility/service/gps/GpsService.h"
#include <Tactility/Mutex.h>
#include <Tactility/Tactility.h>
#include <Tactility/TactilityHeadless.h>
#include <Tactility/Timer.h>
#include <Tactility/service/ServiceContext.h>
#include <Tactility/service/ServiceRegistry.h>
#include <Tactility/service/wifi/Wifi.h>

namespace tt::service::statusbar {

#define TAG "statusbar_service"

// SD card status
#define STATUSBAR_ICON_SDCARD "sdcard.png"
#define STATUSBAR_ICON_SDCARD_ALERT "sdcard_alert.png"

// Wifi status
#define STATUSBAR_ICON_WIFI_OFF_WHITE "wifi_off_white.png"
#define STATUSBAR_ICON_WIFI_SCAN_WHITE "wifi_scan_white.png"
#define STATUSBAR_ICON_WIFI_SIGNAL_WEAK_WHITE "wifi_signal_weak_white.png"
#define STATUSBAR_ICON_WIFI_SIGNAL_MEDIUM_WHITE "wifi_signal_medium_white.png"
#define STATUSBAR_ICON_WIFI_SIGNAL_STRONG_WHITE "wifi_signal_strong_white.png"

// Power status
#define STATUSBAR_ICON_POWER_0 "power_0.png"
#define STATUSBAR_ICON_POWER_10 "power_10.png"
#define STATUSBAR_ICON_POWER_20 "power_20.png"
#define STATUSBAR_ICON_POWER_30 "power_30.png"
#define STATUSBAR_ICON_POWER_40 "power_40.png"
#define STATUSBAR_ICON_POWER_50 "power_50.png"
#define STATUSBAR_ICON_POWER_60 "power_60.png"
#define STATUSBAR_ICON_POWER_70 "power_70.png"
#define STATUSBAR_ICON_POWER_80 "power_80.png"
#define STATUSBAR_ICON_POWER_90 "power_90.png"
#define STATUSBAR_ICON_POWER_100 "power_100.png"

// GPS
#define STATUSBAR_ICON_GPS "location.png"

extern const ServiceManifest manifest;

const char* getWifiStatusIconForRssi(int rssi) {
    if (rssi >= -60) {
        return STATUSBAR_ICON_WIFI_SIGNAL_STRONG_WHITE;
    } else if (rssi >= -70) {
        return STATUSBAR_ICON_WIFI_SIGNAL_MEDIUM_WHITE;
    } else {
        return STATUSBAR_ICON_WIFI_SIGNAL_WEAK_WHITE;
    }
}

static const char* getWifiStatusIcon(wifi::RadioState state, bool secure) {
    int rssi;
    switch (state) {
        using enum wifi::RadioState;
        case On:
        case OnPending:
        case ConnectionPending:
            return STATUSBAR_ICON_WIFI_SCAN_WHITE;
        case OffPending:
        case Off:
            return STATUSBAR_ICON_WIFI_OFF_WHITE;
        case ConnectionActive:
            rssi = wifi::getRssi();
            return getWifiStatusIconForRssi(rssi);
        default:
            tt_crash("not implemented");
    }
}

static const char* getSdCardStatusIcon(hal::sdcard::SdCardDevice::State state) {
    switch (state) {
        using enum hal::sdcard::SdCardDevice::State;
        case Mounted:
            return STATUSBAR_ICON_SDCARD;
        case Error:
        case Unmounted:
        case Unknown:
            return STATUSBAR_ICON_SDCARD_ALERT;
        default:
            tt_crash("Unhandled SdCard state");
    }
}

static _Nullable const char* getPowerStatusIcon() {
    auto get_power = getConfiguration()->hardware->power;
    if (get_power == nullptr) {
        return nullptr;
    }

    auto power = get_power();

    hal::power::PowerDevice::MetricData charge_level;
    if (!power->getMetric(hal::power::PowerDevice::MetricType::ChargeLevel, charge_level)) {
        return nullptr;
    }

    uint8_t charge = charge_level.valueAsUint8;

    if (charge >= 95) {
        return STATUSBAR_ICON_POWER_100;
    } else if (charge >= 85) {
        return STATUSBAR_ICON_POWER_90;
    } else if (charge >= 75) {
        return STATUSBAR_ICON_POWER_80;
    } else if (charge >= 65) {
        return STATUSBAR_ICON_POWER_70;
    } else if (charge >= 55) {
        return STATUSBAR_ICON_POWER_60;
    } else if (charge >= 45) {
        return STATUSBAR_ICON_POWER_50;
    } else if (charge >= 35) {
        return STATUSBAR_ICON_POWER_40;
    } else if (charge >= 25) {
        return STATUSBAR_ICON_POWER_30;
    } else if (charge >= 15) {
        return STATUSBAR_ICON_POWER_20;
    } else if (charge >= 5) {
        return STATUSBAR_ICON_POWER_10;
    } else  {
        return STATUSBAR_ICON_POWER_0;
    }
}

class StatusbarService final : public Service {

private:

    Mutex mutex;
    std::unique_ptr<Timer> updateTimer;
    int8_t gps_icon_id = lvgl::statusbar_icon_add();
    bool gps_last_state = false;
    int8_t wifi_icon_id = lvgl::statusbar_icon_add();
    const char* wifi_last_icon = nullptr;
    int8_t sdcard_icon_id = lvgl::statusbar_icon_add();
    const char* sdcard_last_icon = nullptr;
    int8_t power_icon_id = lvgl::statusbar_icon_add();
    const char* power_last_icon = nullptr;

    std::unique_ptr<service::Paths> paths;

    void lock() const {
        mutex.lock();
    }

    void unlock() const {
        mutex.unlock();
    }

    void updateGpsIcon() {
        auto gps_state = service::gps::findGpsService()->getState();
        bool show_icon = (gps_state == gps::State::OnPending) || (gps_state == gps::State::On);
        if (gps_last_state != show_icon) {
            if (show_icon) {
                auto icon_path = paths->getSystemPathLvgl(STATUSBAR_ICON_GPS);
                lvgl::statusbar_icon_set_image(gps_icon_id, icon_path);
                lvgl::statusbar_icon_set_visibility(gps_icon_id, true);
            } else {
                lvgl::statusbar_icon_set_visibility(gps_icon_id, false);
            }
            gps_last_state = show_icon;
        }
    }

    void updateWifiIcon() {
        wifi::RadioState radio_state = wifi::getRadioState();
        bool is_secure = wifi::isConnectionSecure();
        const char* desired_icon = getWifiStatusIcon(radio_state, is_secure);
        if (wifi_last_icon != desired_icon) {
            if (desired_icon != nullptr) {
                auto icon_path = paths->getSystemPathLvgl(desired_icon);
                lvgl::statusbar_icon_set_image(wifi_icon_id, icon_path);
                lvgl::statusbar_icon_set_visibility(wifi_icon_id, true);
            } else {
                lvgl::statusbar_icon_set_visibility(wifi_icon_id, false);
            }
            wifi_last_icon = desired_icon;
        }
    }

    void updatePowerStatusIcon() {
        const char* desired_icon = getPowerStatusIcon();
        if (power_last_icon != desired_icon) {
            if (desired_icon != nullptr) {
                auto icon_path = paths->getSystemPathLvgl(desired_icon);
                lvgl::statusbar_icon_set_image(power_icon_id, icon_path);
                lvgl::statusbar_icon_set_visibility(power_icon_id, true);
            } else {
                lvgl::statusbar_icon_set_visibility(power_icon_id, false);
            }
            power_last_icon = desired_icon;
        }
    }

    void updateSdCardIcon() {
        auto sdcard = tt::hal::getConfiguration()->sdcard;
        if (sdcard != nullptr) {
            auto state = sdcard->getState();
            if (state != hal::sdcard::SdCardDevice::State::Unknown) {
                auto* desired_icon = getSdCardStatusIcon(state);
                if (sdcard_last_icon != desired_icon) {
                    auto icon_path = paths->getSystemPathLvgl(desired_icon);
                    lvgl::statusbar_icon_set_image(sdcard_icon_id, icon_path);
                    lvgl::statusbar_icon_set_visibility(sdcard_icon_id, true);
                    sdcard_last_icon = desired_icon;
                }
            }
            // TODO: Consider tracking how long the SD card has been in unknown status and then show error
        }
    }

    void update() {
        // TODO: Make thread-safe for LVGL
        updateGpsIcon();
        updateWifiIcon();
        updateSdCardIcon();
        updatePowerStatusIcon();
    }

    static void onUpdate(std::shared_ptr<void> parameter) {
        auto service = std::static_pointer_cast<StatusbarService>(parameter);
        service->update();
    }

public:

    ~StatusbarService() final {
        lvgl::statusbar_icon_remove(wifi_icon_id);
        lvgl::statusbar_icon_remove(sdcard_icon_id);
        lvgl::statusbar_icon_remove(power_icon_id);
    }

    void onStart(ServiceContext& serviceContext) override {
        paths = serviceContext.getPaths();

        // TODO: Make thread-safe for LVGL
        lvgl::statusbar_icon_set_visibility(wifi_icon_id, true);

        auto service = findServiceById(manifest.id);
        assert(service);
        onUpdate(service);

        updateTimer = std::make_unique<Timer>(Timer::Type::Periodic, onUpdate, service);
        // We want to try and scan more often in case of startup or scan lock failure
        updateTimer->start(1000);
    }

    void onStop(ServiceContext& service) override{
        updateTimer->stop();
        updateTimer = nullptr;
    }
};

extern const ServiceManifest manifest = {
    .id = "Statusbar",
    .createService = create<StatusbarService>
};

// endregion service

} // namespace
