#include "Tactility/Check.h"
#include "Tactility/service/gui/Gui_i.h"
#include "Tactility/lvgl/LvglKeypad.h"
#include "Tactility/lvgl/LvglSync.h"

#include <Tactility/TactilityConfig.h>

namespace tt::service::gui {

extern Gui* gui;

static void show_keyboard(lv_event_t* event) {
    lv_obj_t* target = lv_event_get_current_target_obj(event);
    keyboardShow(target);
    lv_obj_scroll_to_view(target, LV_ANIM_ON);
}

static void hide_keyboard(TT_UNUSED lv_event_t* event) {
    keyboardHide();
}

bool keyboardIsEnabled() {
    return !lvgl::keypad_is_available() || TT_CONFIG_FORCE_ONSCREEN_KEYBOARD;
}

void keyboardShow(lv_obj_t* textarea) {
    lock();

    if (gui->keyboard) {
        lv_obj_clear_flag(gui->keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(gui->keyboard, textarea);
    }

    unlock();
}

void keyboardHide() {
    lock();

    if (gui->keyboard) {
        lv_obj_add_flag(gui->keyboard, LV_OBJ_FLAG_HIDDEN);
    }

    unlock();
}

void keyboardAddTextArea(lv_obj_t* textarea) {
    lock();
    tt_check(lvgl::lock(0), "lvgl should already be locked before calling this method");

    if (keyboardIsEnabled()) {
        lv_obj_add_event_cb(textarea, show_keyboard, LV_EVENT_FOCUSED, nullptr);
        lv_obj_add_event_cb(textarea, hide_keyboard, LV_EVENT_DEFOCUSED, nullptr);
        lv_obj_add_event_cb(textarea, hide_keyboard, LV_EVENT_READY, nullptr);
    }

    // lv_obj_t auto-remove themselves from the group when they are destroyed (last checked in LVGL 8.3)
    lv_group_add_obj(gui->keyboardGroup, textarea);

    lvgl::keypad_activate(gui->keyboardGroup);

    lvgl::unlock();
    unlock();
}

} // namespace
