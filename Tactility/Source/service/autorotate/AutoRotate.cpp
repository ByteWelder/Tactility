#include <Tactility/service/autorotate/AutoRotateService.h>
#include <Tactility/service/ServiceManifest.h>
#include <Tactility/service/ServiceRegistration.h>

#include <tactility/delay.h>
#include <tactility/drivers/imu.h>
#include <tactility/log.h>

#include <sdkconfig.h>

#include <lvgl/lvgl.h>

#include <cmath>

#if CONFIG_LV_USE_PERF_MONITOR
// The perf monitor's label and its subject live in LVGL's private display struct - there's no
// public accessor - so appending the IMU readout to that same label needs the private header.
#include <src/display/lv_display_private.h>
#endif

namespace tt::service::autorotate {

using settings::display::Orientation;

constexpr auto* TAG = "autorotate";

#if CONFIG_LV_USE_PERF_MONITOR
// Latest accelerometer reading, rendered as text and appended to the perf monitor label by the
// observer below. Only touched while the LVGL lock is held (see tick()), same as the observer.
static char perf_imu_text[64] = "imu: waiting...";
static lv_observer_t* perf_imu_observer = nullptr;

// LVGL's built-in printf has no float support (LV_USE_FLOAT is off), so a %f would print a literal
// 'f'. Render fixed-point by hand instead: sign, then whole and hundredths as integers.
static void format_g(char* out, size_t out_size, float value) {
    int scaled = static_cast<int>(std::lroundf(value * 100.0F));
    const char* sign = scaled < 0 ? "-" : "+";
    int magnitude = scaled < 0 ? -scaled : scaled;
    lv_snprintf(out, out_size, "%s%d.%02d", sign, magnitude / 100, magnitude % 100);
}

// LVGL's perf monitor owns disp->perf_label and rewrites its text every refresh (~3 Hz) from its
// own observer, which is registered first. This observer is registered after, so it runs right
// after that one and can append the IMU readings to the text it just wrote.
static void perf_imu_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    LV_UNUSED(subject);
    lv_obj_t* label = lv_observer_get_target_obj(observer);
    if (label == nullptr) {
        return;
    }

    // Copy the perf text out first: lv_label_set_text() frees the label's old buffer, which is
    // what lv_label_get_text() points into.
    char combined[192];
    lv_snprintf(combined, sizeof(combined), "%s\n%s", lv_label_get_text(label), perf_imu_text);
    lv_label_set_text(label, combined);
}
#endif

void AutoRotateService::tick() {
    if (settingsReloadRequested.exchange(false, std::memory_order_acq_rel)) {
        auto reloaded = settings::display::loadOrGetDefault();
        if (!lvgl_try_lock(100)) {
            // Put the request back - a reloadSettings() call racing this exchange must not be lost.
            settingsReloadRequested.store(true, std::memory_order_release);
            return;
        }
        cachedDisplaySettings = reloaded;
        orientationMath.reset(settings::display::toLvglDisplayRotation(cachedDisplaySettings.orientation));
        lvgl_unlock();
    }

#if CONFIG_LV_USE_PERF_MONITOR
    constexpr bool debug_enabled = true;
#else
    constexpr bool debug_enabled = false;
#endif

    // The IMU readout appended to the perf monitor needs the IMU even when auto-rotate is off.
    if (!cachedDisplaySettings.autoRotateEnabled && !debug_enabled) {
        return;
    }

    ::Device* imu;
    if (device_get_first_active_by_type(&IMU_TYPE, &imu) != ERROR_NONE) {
        return;
    }

    ImuAccelData accel;
    error_t result = imu_read_accel(imu, &accel);
    device_put(imu);
    if (result != ERROR_NONE) {
        return;
    }

    if (!lvgl_try_lock(100)) {
        return; // Retry on next tick
    }

#if CONFIG_LV_USE_PERF_MONITOR
    char ax_str[16];
    char ay_str[16];
    char az_str[16];
    format_g(ax_str, sizeof(ax_str), accel.ax);
    format_g(ay_str, sizeof(ay_str), accel.ay);
    format_g(az_str, sizeof(az_str), accel.az);
    lv_snprintf(perf_imu_text, sizeof(perf_imu_text), "ax %s ay %s\naz %s", ax_str, ay_str, az_str);

    // The label only refreshes when the perf monitor ticks (~3 Hz); log once a second so the
    // serial console stays readable while still capturing exact values.
    static uint8_t log_divider = 0;
    if (++log_divider >= 10) {
        log_divider = 0;
        LOG_I(TAG, "accel ax=%s ay=%s az=%s", ax_str, ay_str, az_str);
    }
#endif

    if (!cachedDisplaySettings.autoRotateEnabled) {
        lvgl_unlock();
        return;
    }

    int mount_quarter_turns = static_cast<int>(cachedDisplaySettings.autoRotateMountRotation);
    lv_display_rotation_t rotation = orientationMath.update(accel.ax, accel.ay, mount_quarter_turns);
    Orientation orientation = settings::display::fromLvglDisplayRotation(rotation);
    if (orientation == cachedDisplaySettings.orientation) {
        lvgl_unlock();
        return;
    }

    lv_display_set_rotation(lv_display_get_default(), rotation);
    lvgl_unlock();

    // TODO: Display settings app might be saving DisplaySettings at the same time. This might corrupt the settings file.
    auto settings_to_save = settings::display::loadOrGetDefault();
    settings_to_save.orientation = orientation;
    cachedDisplaySettings.orientation = orientation;
    settings::display::save(settings_to_save);
}

bool AutoRotateService::onStart(ServiceContext& service) {
    cachedDisplaySettings = settings::display::loadOrGetDefault();
    if (lvgl_try_lock(100)) {
        orientationMath.reset(settings::display::toLvglDisplayRotation(cachedDisplaySettings.orientation));
#if CONFIG_LV_USE_PERF_MONITOR
        // Hook the perf monitor label once. The perf monitor is created alongside the display,
        // which exists before services start, so its label/subject are ready here.
        lv_display_t* display = lv_display_get_default();
        if (display != nullptr && display->perf_label != nullptr && perf_imu_observer == nullptr) {
            perf_imu_observer = lv_subject_add_observer_obj(
                &display->perf_sysmon_backend.subject, perf_imu_observer_cb, display->perf_label, nullptr);
        }
#endif
        lvgl_unlock();
    } else {
        // Lock failed - defer the sync to tick()'s reload path instead of a mismatched baseline.
        settingsReloadRequested.store(true, std::memory_order_release);
    }

    timer = std::make_unique<Timer>(Timer::Type::Periodic, millis_to_ticks(TICK_INTERVAL_MS), [this]{ this->tick(); });
    timer->setCallbackPriority(Thread::Priority::Lower);
    timer->start();
    return true;
}

void AutoRotateService::onStop(ServiceContext& service) {
    if (timer) {
        timer->stop();
        timer = nullptr;
    }

#if CONFIG_LV_USE_PERF_MONITOR
    if (perf_imu_observer != nullptr) {
        if (lvgl_try_lock(100)) {
            lv_observer_remove(perf_imu_observer);
            perf_imu_observer = nullptr;
            lvgl_unlock();
        }
    }
#endif
}

void AutoRotateService::reloadSettings() {
    settingsReloadRequested.store(true, std::memory_order_release);
}

std::shared_ptr<AutoRotateService> findService() {
    return std::static_pointer_cast<AutoRotateService>(
        findServiceById("tactility.autorotate")
    );
}

extern const ServiceManifest manifest = {
    .id = "tactility.autorotate",
    .createService = create<AutoRotateService>
};

} // namespace tt::service::autorotate
