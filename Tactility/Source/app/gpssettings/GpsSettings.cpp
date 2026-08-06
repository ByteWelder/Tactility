#include <lvgl/icons/shared.h>
#include <lvgl/lvgl.h>

#include <Tactility/Tactility.h>
#include <Tactility/Timer.h>
#include <Tactility/app/AppManifest.h>
#include <Tactility/app/alertdialog/AlertDialog.h>
#include <Tactility/lvgl/Toolbar.h>

#include <tactility/device.h>
#include <tactility/time.h>

#include <atomic>
#include <cstring>
#include <lvgl.h>
#include <string>
#include <vector>

#include <gps/gps.h>
#include <gps/gps_settings.h>

namespace tt::app::addgps {
extern AppManifest manifest;
}

namespace tt::app::gpssettings {

extern const AppManifest manifest;

class GpsSettingsApp final : public App {

    struct DeviceRow {
        Device* device;
        lv_obj_t* button;
        lv_obj_t* buttonLabel;
        bool hasConfiguration = false;
        size_t configurationIndex = 0;
    };

    std::unique_ptr<Timer> timer;
    lv_obj_t* deviceListWrapper = nullptr;
    std::vector<DeviceRow> deviceRows;
    std::atomic<bool> isShown = false;
    bool hasPendingDelete = false;
    Device* pendingDeleteDevice = nullptr;
    size_t pendingDeleteIndex = 0;

    static void onAddGpsCallback(lv_event_t* event) {
        auto* app = (GpsSettingsApp*)lv_event_get_user_data(event);
        app->onAddGps();
    }

    void onAddGps() {
        app::start(addgps::manifest.appId);
    }

    static void onDeviceButtonCallback(lv_event_t* event) {
        auto* button = lv_event_get_target_obj(event);
        auto* device = static_cast<Device*>(lv_obj_get_user_data(button));

        bool running = device_is_ready(device);
        // device_start()/device_stop() are potentially blocking calls, so use a dispatcher to not block the UI
        getMainDispatcher().dispatch([device, running] {
            if (running) {
                device_stop(device);
            } else {
                device_start(device);
            }
        });
    }

    // Finds the persisted configuration backing `device` (matched by its parent UART's name)
    // and returns its index into gps_settings_for_each_configuration()'s ordering - the handle
    // gps_settings_remove_configuration_at() needs to delete exactly this entry, even if another
    // entry happens to have identical field values.
    // Devicetree-declared GPS_TYPE devices have no such configuration and never match.
    static bool findConfigurationIndexForDevice(Device* device, size_t& outIndex) {
        auto* parent = device_get_parent(device);
        if (parent == nullptr) {
            return false;
        }

        struct Context {
            const char* uartName;
            size_t* outIndex;
            bool found;
        } context = { parent->name, &outIndex, false };

        gps_settings_for_each_configuration(&context, [](const GpsConfiguration* configuration, size_t index, void* untyped_context) {
            auto* ctx = static_cast<Context*>(untyped_context);
            if (!ctx->found && strcmp(configuration->uart_name, ctx->uartName) == 0) {
                *ctx->outIndex = index;
                ctx->found = true;
            }
        });

        return context.found;
    }

    static void onDeleteButtonCallback(lv_event_t* event) {
        auto* app = static_cast<GpsSettingsApp*>(lv_event_get_user_data(event));
        auto* button = lv_event_get_target_obj(event);
        auto* device = static_cast<Device*>(lv_obj_get_user_data(button));
        app->onDeleteDevice(device);
    }

    void onDeleteDevice(Device* device) {
        for (auto& row : deviceRows) {
            if (row.device == device && row.hasConfiguration) {
                pendingDeleteDevice = device;
                pendingDeleteIndex = row.configurationIndex;
                hasPendingDelete = true;
                alertdialog::start("Confirmation", std::string("Do you want to delete ") + device->name + "?", std::vector<std::string> { "Yes", "No" });
                return;
            }
        }
    }

    void createDeviceRow(Device* device) {
        auto* wrapper = lv_obj_create(deviceListWrapper);
        lv_obj_set_size(wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_border_width(wrapper, 0, 0);
        lv_obj_set_style_pad_all(wrapper, 0, 0);

        auto* name_label = lv_label_create(wrapper);
        char model_name[64];
        if (gps_get_model_name(device, model_name, sizeof(model_name)) == ERROR_NONE) {
            lv_label_set_text(name_label, model_name);
        } else {
            lv_label_set_text(name_label, device->name);
        }

        auto* actions_wrapper = lv_obj_create(wrapper);
        lv_obj_set_size(actions_wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(actions_wrapper, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_border_width(actions_wrapper, 0, 0);
        lv_obj_set_style_pad_all(actions_wrapper, 0, 0);
        lv_obj_set_style_pad_column(actions_wrapper, 4, 0);

        auto* button = lv_button_create(actions_wrapper);
        lv_obj_add_event_cb(button, onDeviceButtonCallback, LV_EVENT_SHORT_CLICKED, this);
        lv_obj_set_user_data(button, device);
        auto* button_label = lv_label_create(button);
        lv_label_set_text(button_label, "Start");

        DeviceRow row { .device = device, .button = button, .buttonLabel = button_label };

        // Only devices backed by a persisted configuration (not devicetree-declared ones) can be deleted.
        size_t configurationIndex;
        if ((device->flags & DEVICE_FLAG_DYNAMIC) && findConfigurationIndexForDevice(device, configurationIndex)) {
            auto* delete_button = lv_button_create(actions_wrapper);
            lv_obj_add_event_cb(delete_button, onDeleteButtonCallback, LV_EVENT_SHORT_CLICKED, this);
            lv_obj_set_user_data(delete_button, device);
            auto* delete_label = lv_label_create(delete_button);
            lv_label_set_text(delete_label, LVGL_ICON_SHARED_DELETE);

            row.hasConfiguration = true;
            row.configurationIndex = configurationIndex;
        }

        deviceRows.push_back(row);
    }

    // Rebuilds the device list. Only needs to run when the set of devices could've changed
    // (on show, and after returning from AddGpsApp) - button state itself is refreshed by the timer.
    void rebuildDeviceList() {
        lv_obj_clean(deviceListWrapper);
        deviceRows.clear();

        device_for_each_of_type(&GPS_TYPE, this, [](Device* device, void* context) {
            static_cast<GpsSettingsApp*>(context)->createDeviceRow(device);
            return true;
        });
    }

    void updateDeviceStates() {
        lvgl_lock();
        for (const auto& row : deviceRows) {
            const char* text = "Start";
            bool enabled = true;

            if (device_is_ready(row.device)) {
                switch (gps_get_state(row.device)) {
                    case GPS_STATE_PENDING_ON:
                        text = "Starting...";
                        enabled = false;
                        break;
                    case GPS_STATE_PENDING_OFF:
                        text = "Stopping...";
                        enabled = false;
                        break;
                    default:
                        text = "Stop";
                        enabled = true;
                        break;
                }
            }
            lv_label_set_text(row.buttonLabel, text);
            if (enabled) {
                lv_obj_remove_state(row.button, LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(row.button, LV_STATE_DISABLED);
            }
        }
        lvgl_unlock();
    }

public:

    GpsSettingsApp() {
        // Runs while the screen is shown - there's no push notification for GPS device state
        // changes, so this is the only way this screen finds out about them.
        timer = std::make_unique<Timer>(Timer::Type::Periodic, seconds_to_ticks(1), [this] {
            updateDeviceStates();
        });
    }

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

        uint8_t margin = (lvgl_get_ui_density() == LVGL_UI_DENSITY_COMPACT) ? 2 : 8;

        auto* toolbar = lvgl::toolbar_create(parent, app);
        lvgl_toolbar_add_text_button_action(toolbar, LV_SYMBOL_PLUS, onAddGpsCallback, this);
        lv_obj_set_style_margin_bottom(toolbar, margin, LV_STATE_DEFAULT);

        deviceListWrapper = lv_obj_create(parent);
        lv_obj_set_size(deviceListWrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(deviceListWrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_grow(deviceListWrapper, 1);
        lv_obj_set_style_border_width(deviceListWrapper, 0, 0);
        lv_obj_set_style_pad_hor(deviceListWrapper, margin, 0);
        lv_obj_set_style_pad_top(deviceListWrapper, 0, 0);
        lv_obj_set_style_pad_bottom(deviceListWrapper,  margin, 0);
        lv_obj_set_style_pad_row(deviceListWrapper, margin, 0);

        rebuildDeviceList();

        timer->start();
        updateDeviceStates();

        // Only after deviceListWrapper is fully built: onResult() (Loader thread) checks
        // this before touching it, since it can run before or after this onShow() call.
        isShown = true;
    }

    void onHide(AppContext& app) override {
        isShown = false;
        timer->stop();
    }

    void onResult(AppContext&, LaunchId, Result result, std::unique_ptr<Bundle> bundle) override {
        if (!hasPendingDelete) {
            return;
        }
        hasPendingDelete = false;

        if (result != Result::Ok || bundle == nullptr || alertdialog::getResultIndex(*bundle) != 0) { // 0 = Yes
            return;
        }

        // This runs on the Loader thread, concurrently with the periodic timer callback
        // (updateDeviceStates(), timer daemon thread) and possibly with onShow() (GUI
        // thread). Take the same lock updateDeviceStates() uses and hold it across the
        // free below, so the timer can never observe pendingDeleteDevice as a dangling
        // pointer in deviceRows.
        lvgl_lock();
        // Drop the stale row unconditionally (cheap vector op, no LVGL calls) - this is
        // what keeps the timer safe regardless of whether onShow() has run yet this cycle.
        std::erase_if(deviceRows, [this](const DeviceRow& row) {
            return row.device == pendingDeleteDevice;
        });
        lvgl_unlock();

        // gps_settings_remove_configuration_at() frees the underlying Device synchronously -
        // do this only after the dangling pointer is already out of deviceRows.
        gps_settings_remove_configuration_at(pendingDeleteIndex);
        pendingDeleteDevice = nullptr;

        // Only safe to touch deviceListWrapper if onShow() already built it for this show
        // cycle - it may not have run yet, in which case it'll rebuild fresh (post-deletion,
        // deviceRows already correct) when it does.
        lvgl_lock();
        if (isShown) {
            rebuildDeviceList();
        }
        lvgl_unlock();
    }
};

extern const AppManifest manifest = {
    .appId = "GpsSettings",
    .appName = "GPS",
    .appIcon = LVGL_ICON_SHARED_NAVIGATION,
    .appCategory = Category::Settings,
    .createApp = create<GpsSettingsApp>
};

void start() {
    app::start(manifest.appId);
}

} // namespace
