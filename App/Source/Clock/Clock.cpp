#define LV_USE_PRIVATE_API 1

#include <Tactility/app/AppManifest.h>
#include <Tactility/app/AppContext.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/settings/Time.h>
#include <Tactility/Preferences.h>
#include <lvgl.h>
#include <ctime>
#include <cmath>

#ifdef ESP_PLATFORM
#include "Tactility/Timer.h"
#include "Tactility/lvgl/LvglSync.h"
#include "esp_sntp.h"
#endif

namespace tt::app::clock {

class ClockApp : public App {
private:
    struct AppWrapper {
        ClockApp* app;
        AppWrapper(ClockApp* app) : app(app) {}
    };

    lv_obj_t* toolbar;
    lv_obj_t* clock_container;
    lv_obj_t* time_label; // Digital
    lv_obj_t* clock_face; // Analog
    lv_obj_t* hour_hand;
    lv_obj_t* minute_hand;
    lv_obj_t* second_hand;
    lv_obj_t* wifi_label;
    lv_obj_t* wifi_button;
#ifdef ESP_PLATFORM
    std::unique_ptr<Timer> timer; // Only declare timer for ESP
#endif
    bool is_analog;
    AppContext* context;

#ifdef ESP_PLATFORM
    static void timer_callback(std::shared_ptr<void> appWrapper) {
        auto* app = std::static_pointer_cast<AppWrapper>(appWrapper)->app;
        TT_LOG_I("Clock", "Timer fired");
        app->update_time();
    }
#endif

    static void toggle_mode_cb(lv_event_t* e) {
        ClockApp* app = static_cast<ClockApp*>(lv_event_get_user_data(e));
        app->toggle_mode();
    }

    static void wifi_connect_cb(lv_event_t* e) {
        tt::app::start("WifiManage");
    }

    void load_mode() {
        tt::Preferences prefs("clock_settings");
        is_analog = false;
        prefs.optBool("is_analog", is_analog);
    }

    void save_mode() {
        tt::Preferences prefs("clock_settings");
        prefs.putBool("is_analog", is_analog);
    }

    void toggle_mode() {
        is_analog = !is_analog;
        save_mode();
        redraw_clock();
    }

#ifdef ESP_PLATFORM
    bool is_time_synced() {
        return sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED;
    }

    void update_time() {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        if (!lock.lock(lvgl::defaultLockTime)) {
            TT_LOG_E("Clock", "LVGL lock failed in update_time");
            return;
        }

        if (!is_time_synced()) {
            if (wifi_label) lv_label_set_text(wifi_label, "No Wi-Fi - Time not synced");
            return;
        }

        time_t now;
        struct tm timeinfo;
        ::time(&now);
        localtime_r(&now, &timeinfo);

        if (is_analog) {
            float hour_angle = (timeinfo.tm_hour % 12 + timeinfo.tm_min / 60.0f) * 30.0f - 90;
            float minute_angle = timeinfo.tm_min * 6.0f - 90;
            float second_angle = timeinfo.tm_sec * 6.0f - 90;

            lv_point_precise_t hour_points[] = {{0, 0}, {static_cast<lv_coord_t>(50 * cos(hour_angle * M_PI / 180)), static_cast<lv_coord_t>(50 * sin(hour_angle * M_PI / 180))}};
            lv_point_precise_t minute_points[] = {{0, 0}, {static_cast<lv_coord_t>(70 * cos(minute_angle * M_PI / 180)), static_cast<lv_coord_t>(70 * sin(minute_angle * M_PI / 180))}};
            lv_point_precise_t second_points[] = {{0, 0}, {static_cast<lv_coord_t>(80 * cos(second_angle * M_PI / 180)), static_cast<lv_coord_t>(80 * sin(second_angle * M_PI / 180))}};

            lv_line_set_points(hour_hand, hour_points, 2);
            lv_line_set_points(minute_hand, minute_points, 2);
            lv_line_set_points(second_hand, second_points, 2);
        } else {
            char time_str[16];
            if (settings::isTimeFormat24Hour()) {
                strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
            } else {
                strftime(time_str, sizeof(time_str), "%I:%M:%S %p", &timeinfo);
            }
            lv_label_set_text(time_label, time_str);
        }
    }
#else
    bool is_time_synced() {
        return true; // Simulator assumes synced
    }

    void update_time() {
        // No-op for simulator; static message handled in redraw_clock
    }
#endif

    void redraw_clock() {
#ifdef ESP_PLATFORM
        auto lock = lvgl::getSyncLock()->asScopedLock();
        if (!lock.lock(lvgl::defaultLockTime)) {
            TT_LOG_E("Clock", "LVGL lock failed in redraw_clock");
            return;
        }
#endif

        lv_obj_clean(clock_container);
        time_label = nullptr;
        clock_face = hour_hand = minute_hand = second_hand = nullptr;
        wifi_label = nullptr;
        wifi_button = nullptr;

#ifdef ESP_PLATFORM
        if (!is_time_synced()) {
            wifi_label = lv_label_create(clock_container);
            lv_label_set_text(wifi_label, "No Wi-Fi - Time not synced");
            lv_obj_align(wifi_label, LV_ALIGN_CENTER, 0, -20);

            wifi_button = lv_btn_create(clock_container);
            lv_obj_t* btn_label = lv_label_create(wifi_button);
            lv_label_set_text(btn_label, "Connect to Wi-Fi");
            lv_obj_center(btn_label);
            lv_obj_align(wifi_button, LV_ALIGN_CENTER, 0, 20);
            lv_obj_add_event_cb(wifi_button, wifi_connect_cb, LV_EVENT_CLICKED, context);
        } else if (is_analog) {
            clock_face = lv_arc_create(clock_container);
            lv_arc_set_range(clock_face, 0, 360);
            lv_arc_set_bg_angles(clock_face, 0, 360);
            lv_obj_set_size(clock_face, 200, 200);
            lv_obj_center(clock_face);
            lv_obj_set_style_arc_width(clock_face, 2, 0);

            hour_hand = lv_line_create(clock_face);
            minute_hand = lv_line_create(clock_face);
            second_hand = lv_line_create(clock_face);
            lv_obj_set_style_line_width(hour_hand, 4, 0);
            lv_obj_set_style_line_width(minute_hand, 3, 0);
            lv_obj_set_style_line_width(second_hand, 1, 0);
            lv_obj_set_style_line_color(second_hand, lv_palette_main(LV_PALETTE_RED), 0);
            update_time();
        } else {
            time_label = lv_label_create(clock_container);
            lv_label_set_text(time_label, "Loading...");
            lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
            update_time();
        }
#else
        // Simulator: show static message
        time_label = lv_label_create(clock_container);
        lv_label_set_text(time_label, "Clock not supported in simulator");
        lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
#endif
    }

public:
    void onShow(AppContext& app_context, lv_obj_t* parent) override {
        context = &app_context;
        toolbar = tt::lvgl::toolbar_create(parent, app_context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t* toggle_btn = lv_btn_create(toolbar);
        lv_obj_t* toggle_label = lv_label_create(toggle_btn);
        lv_label_set_text(toggle_label, "Toggle Mode");
        lv_obj_center(toggle_label);
        lv_obj_align(toggle_btn, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_add_event_cb(toggle_btn, toggle_mode_cb, LV_EVENT_CLICKED, this);

        clock_container = lv_obj_create(parent);
        lv_obj_set_size(clock_container, LV_PCT(100), LV_PCT(80));
        lv_obj_align_to(clock_container, toolbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

        load_mode();
        redraw_clock();

#ifdef ESP_PLATFORM
        auto wrapper = std::make_shared<AppWrapper>(this);
        timer = std::make_unique<Timer>(Timer::Type::Periodic, [wrapper]() { timer_callback(wrapper); });
        timer->start(1000);
        TT_LOG_I("Clock", "Timer started in onShow");
#endif
    }

    void onHide(AppContext& app_context) override {
#ifdef ESP_PLATFORM
        timer->stop();
        TT_LOG_I("Clock", "Timer stopped in onHide");
#endif
    }
};

extern const AppManifest clock_app = {
    .id = "Clock",
    .name = "Clock",
    .createApp = create<ClockApp>
};

} // namespace tt::app::clock
