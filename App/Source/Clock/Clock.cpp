#define LV_USE_PRIVATE_API 1


#include <Tactility/app/AppManifest.h>
#include <Tactility/app/AppContext.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/time/Time.h>
#include <Tactility/Preferences.h>
#include <lvgl.h>
#include <ctime>
#include <cmath>
// #include "esp_sntp.h"

using namespace tt::app;

class ClockApp : public App {
private:
    lv_obj_t* toolbar;
    lv_obj_t* clock_container;
    lv_obj_t* time_label;      // Digital
    lv_obj_t* clock_face;      // Analog
    lv_obj_t* hour_hand;
    lv_obj_t* minute_hand;
    lv_obj_t* second_hand;
    lv_obj_t* wifi_label;
    lv_obj_t* wifi_button;
    lv_timer_t* timer;
    bool is_analog;
    AppContext* context;       // Store context for Wi-Fi button

    static void timer_callback(lv_timer_t* timer) {
        ClockApp* app = static_cast<ClockApp*>(lv_timer_get_user_data(timer));
        app->update_time();
    }

    static void toggle_mode_cb(lv_event_t* e) {
        ClockApp* app = static_cast<ClockApp*>(lv_event_get_user_data(e));
        app->toggle_mode();
    }

    static void wifi_connect_cb(lv_event_t* e) {
        tt::app::start("WifiManage");  // Static call, no context needed
    }

    void load_mode() {
        tt::Preferences prefs("clock_settings");
        is_analog = false; // Default digital
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

    bool is_time_synced() {
        return sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED;
    }

    void update_time() {
        if (!is_time_synced()) {
            if (wifi_label) lv_label_set_text(wifi_label, "No Wi-Fi - Time not synced");
            return;
        }

        time_t now;
        struct tm timeinfo;
        time(&now);
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
            if (tt::time::isTimeFormat24Hour()) {
                strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
            } else {
                strftime(time_str, sizeof(time_str), "%I:%M:%S %p", &timeinfo);
            }
            lv_label_set_text(time_label, time_str);
        }
    }

    void redraw_clock() {
        lv_obj_clean(clock_container);
        time_label = nullptr;
        clock_face = hour_hand = minute_hand = second_hand = nullptr;
        wifi_label = nullptr;
        wifi_button = nullptr;

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
            return;
        }

        if (is_analog) {
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
        } else {
            time_label = lv_label_create(clock_container);
            lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
        }
        update_time();
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

        timer = lv_timer_create(timer_callback, 1000, this);
    }

    void onHide(AppContext& app_context) override {
        if (timer) {
            lv_timer_del(timer);
            timer = nullptr;
        }
    }
};

extern const AppManifest clock_app = {
    .id = "Clock",
    .name = "Clock",
    .createApp = create<ClockApp>
};
