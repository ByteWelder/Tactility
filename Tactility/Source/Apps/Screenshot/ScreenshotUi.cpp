#include "ScreenshotUi.h"

#include "TactilityCore.h"
#include "Hal/Sdcard.h"
#include "Services/Gui/Gui.h"
#include "Services/Screenshot/Screenshot.h"
#include "Ui/Toolbar.h"

namespace tt::app::screenshot {

#define TAG "screenshot_ui"

static void update_mode(ScreenshotUi* ui) {
    lv_obj_t* label = ui->start_stop_button_label;
    if (service::screenshot::is_started()) {
        lv_label_set_text(label, "Stop");
    } else {
        lv_label_set_text(label, "Start");
    }

    uint32_t selected = lv_dropdown_get_selected(ui->mode_dropdown);
    if (selected == 0) { // Timer
        lv_obj_remove_flag(ui->timer_wrapper, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui->timer_wrapper, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_mode_set(lv_event_t* event) {
    auto* ui = (ScreenshotUi*)lv_event_get_user_data(event);
    update_mode(ui);
}

static void on_start_pressed(lv_event_t* event) {
    auto* ui = static_cast<ScreenshotUi*>(lv_event_get_user_data(event));

    if (service::screenshot::is_started()) {
        TT_LOG_I(TAG, "Stop screenshot");
        service::screenshot::stop();
    } else {
        uint32_t selected = lv_dropdown_get_selected(ui->mode_dropdown);
        const char* path = lv_textarea_get_text(ui->path_textarea);
        if (selected == 0) {
            TT_LOG_I(TAG, "Start timed screenshots");
            const char* delay_text = lv_textarea_get_text(ui->delay_textarea);
            int delay = atoi(delay_text);
            if (delay > 0) {
                service::screenshot::start_timed(path, delay, 1);
            } else {
                TT_LOG_W(TAG, "Ignored screenshot start because delay was 0");
            }
        } else {
            TT_LOG_I(TAG, "Start app screenshots");
            service::screenshot::start_apps(path);
        }
    }

    update_mode(ui);
}

static void create_mode_setting_ui(ScreenshotUi* ui, lv_obj_t* parent) {
    lv_obj_t* mode_wrapper = lv_obj_create(parent);
    lv_obj_set_size(mode_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(mode_wrapper, 0, 0);
    lv_obj_set_style_border_width(mode_wrapper, 0, 0);

    lv_obj_t* mode_label = lv_label_create(mode_wrapper);
    lv_label_set_text(mode_label, "Mode:");
    lv_obj_align(mode_label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* mode_dropdown = lv_dropdown_create(mode_wrapper);
    lv_dropdown_set_options(mode_dropdown, "Timer\nApp start");
    lv_obj_align_to(mode_dropdown, mode_label, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    lv_obj_add_event_cb(mode_dropdown, on_mode_set, LV_EVENT_VALUE_CHANGED, ui);
    ui->mode_dropdown = mode_dropdown;
    service::screenshot::ScreenshotMode mode = service::screenshot::get_mode();
    if (mode == service::screenshot::ScreenshotModeApps) {
        lv_dropdown_set_selected(mode_dropdown, 1);
    }

    lv_obj_t* button = lv_button_create(mode_wrapper);
    lv_obj_align(button, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_t* button_label = lv_label_create(button);
    lv_obj_align(button_label, LV_ALIGN_CENTER, 0, 0);
    ui->start_stop_button_label = button_label;
    lv_obj_add_event_cb(button, &on_start_pressed, LV_EVENT_CLICKED, ui);
}

static void create_path_ui(ScreenshotUi* ui, lv_obj_t* parent) {
    lv_obj_t* path_wrapper = lv_obj_create(parent);
    lv_obj_set_size(path_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(path_wrapper, 0, 0);
    lv_obj_set_style_border_width(path_wrapper, 0, 0);
    lv_obj_set_flex_flow(path_wrapper, LV_FLEX_FLOW_ROW);

    lv_obj_t* label_wrapper = lv_obj_create(path_wrapper);
    lv_obj_set_style_border_width(label_wrapper, 0, 0);
    lv_obj_set_style_pad_all(label_wrapper, 0, 0);
    lv_obj_set_size(label_wrapper, 44, 36);
    lv_obj_t* path_label = lv_label_create(label_wrapper);
    lv_label_set_text(path_label, "Path:");
    lv_obj_align(path_label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* path_textarea = lv_textarea_create(path_wrapper);
    lv_textarea_set_one_line(path_textarea, true);
    lv_obj_set_flex_grow(path_textarea, 1);
    ui->path_textarea = path_textarea;
    if (get_platform() == PlatformEsp) {
        if (hal::sdcard::get_state() == hal::sdcard::StateMounted) {
            lv_textarea_set_text(path_textarea, "A:/sdcard");
        } else {
            lv_textarea_set_text(path_textarea, "Error: no SD card");
        }
    } else { // PC
        lv_textarea_set_text(path_textarea, "A:");
    }
}

static void create_timer_settings_ui(ScreenshotUi* ui, lv_obj_t* parent) {
    lv_obj_t* timer_wrapper = lv_obj_create(parent);
    lv_obj_set_size(timer_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(timer_wrapper, 0, 0);
    lv_obj_set_style_border_width(timer_wrapper, 0, 0);
    ui->timer_wrapper = timer_wrapper;

    lv_obj_t* delay_wrapper = lv_obj_create(timer_wrapper);
    lv_obj_set_size(delay_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(delay_wrapper, 0, 0);
    lv_obj_set_style_border_width(delay_wrapper, 0, 0);
    lv_obj_set_flex_flow(delay_wrapper, LV_FLEX_FLOW_ROW);

    lv_obj_t* delay_label_wrapper = lv_obj_create(delay_wrapper);
    lv_obj_set_style_border_width(delay_label_wrapper, 0, 0);
    lv_obj_set_style_pad_all(delay_label_wrapper, 0, 0);
    lv_obj_set_size(delay_label_wrapper, 44, 36);
    lv_obj_t* delay_label = lv_label_create(delay_label_wrapper);
    lv_label_set_text(delay_label, "Delay:");
    lv_obj_align(delay_label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* delay_textarea = lv_textarea_create(delay_wrapper);
    lv_textarea_set_one_line(delay_textarea, true);
    lv_textarea_set_accepted_chars(delay_textarea, "0123456789");
    lv_textarea_set_text(delay_textarea, "10");
    lv_obj_set_flex_grow(delay_textarea, 1);
    ui->delay_textarea = delay_textarea;

    lv_obj_t* delay_unit_label_wrapper = lv_obj_create(delay_wrapper);
    lv_obj_set_style_border_width(delay_unit_label_wrapper, 0, 0);
    lv_obj_set_style_pad_all(delay_unit_label_wrapper, 0, 0);
    lv_obj_set_size(delay_unit_label_wrapper, LV_SIZE_CONTENT, 36);
    lv_obj_t* delay_unit_label = lv_label_create(delay_unit_label_wrapper);
    lv_obj_align(delay_unit_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(delay_unit_label, "seconds");
}

void create_ui(App app, ScreenshotUi* ui, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_t* toolbar = lvgl::toolbar_create(parent, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

    create_mode_setting_ui(ui, wrapper);
    create_path_ui(ui, wrapper);
    create_timer_settings_ui(ui, wrapper);

    service::gui::keyboard_add_textarea(ui->delay_textarea);
    service::gui::keyboard_add_textarea(ui->path_textarea);

    update_mode(ui);
}

} // namespace
