#include "gui_i.h"

#include "tactility_config.h"
#include "ui/lvgl_keypad.h"
#include "ui/lvgl_sync.h"

extern Gui* gui;

static void show_keyboard(lv_event_t* event) {
    gui_keyboard_show(event->current_target);
    lv_obj_scroll_to_view(event->current_target, LV_ANIM_ON);
}

static void hide_keyboard(TT_UNUSED lv_event_t* event) {
    gui_keyboard_hide();
}

bool gui_keyboard_is_enabled() {
    return !tt_lvgl_keypad_is_available() || TT_CONFIG_FORCE_ONSCREEN_KEYBOARD;
}

void gui_keyboard_show(lv_obj_t* textarea) {
    gui_lock();

    if (gui->keyboard) {
        lv_obj_clear_flag(gui->keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(gui->keyboard, textarea);

        if (gui->toolbar) {
            lv_obj_add_flag(gui->toolbar, LV_OBJ_FLAG_HIDDEN);
        }
    }

    gui_unlock();
}

void gui_keyboard_hide() {
    gui_lock();

    if (gui->keyboard) {
        lv_obj_add_flag(gui->keyboard, LV_OBJ_FLAG_HIDDEN);
        if (gui->toolbar) {
            lv_obj_clear_flag(gui->toolbar, LV_OBJ_FLAG_HIDDEN);
        }
    }

    gui_unlock();
}

void gui_keyboard_add_textarea(lv_obj_t* textarea) {
    gui_lock();
    tt_check(tt_lvgl_lock(0), "lvgl should already be locked before calling this method");

    if (gui_keyboard_is_enabled()) {
        lv_obj_add_event_cb(textarea, show_keyboard, LV_EVENT_FOCUSED, NULL);
        lv_obj_add_event_cb(textarea, hide_keyboard, LV_EVENT_DEFOCUSED, NULL);
        lv_obj_add_event_cb(textarea, hide_keyboard, LV_EVENT_READY, NULL);
    }

    // lv_obj_t auto-remove themselves from the group when they are destroyed (last checked in LVGL 8.3)
    lv_group_add_obj(gui->keyboard_group, textarea);

    tt_lvgl_keypad_activate(gui->keyboard_group);

    tt_lvgl_unlock();
    gui_unlock();
}
