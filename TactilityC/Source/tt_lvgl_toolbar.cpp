#include "tt_lvgl_toolbar.h"
#include <Tactility/lvgl/Toolbar.h>

extern "C" {

lv_obj_t* tt_lvgl_toolbar_create(lv_obj_t* parent, const char* title) {
    return tt::lvgl::toolbar_create(parent, title);
}

lv_obj_t* tt_lvgl_toolbar_create_for_app(lv_obj_t* parent, AppHandle context) {
    return tt::lvgl::toolbar_create(parent, *(tt::app::AppContext*)context);
}

void toolbar_set_title(lv_obj_t* obj, const char* title) {
    tt::lvgl::toolbar_set_title(obj, title);
}

void toolbar_set_nav_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* callbackEventUserData) {
    tt::lvgl::toolbar_set_nav_action(obj, icon, callback, callbackEventUserData);
}

lv_obj_t* toolbar_add_button_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* callbackEventUserData) {
    return tt::lvgl::toolbar_add_button_action(obj, icon, callback, callbackEventUserData);
}

lv_obj_t* toolbar_add_switch_action(lv_obj_t* obj) {
    return tt::lvgl::toolbar_add_switch_action(obj);
}

lv_obj_t* toolbar_add_spinner_action(lv_obj_t* obj) {
    return tt::lvgl::toolbar_add_spinner_action(obj);
}

}
