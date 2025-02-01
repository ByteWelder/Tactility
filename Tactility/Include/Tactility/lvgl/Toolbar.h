#pragma once

#include "lvgl.h"
#include "../app/AppContext.h"

namespace tt::lvgl {

#define TOOLBAR_HEIGHT 40
#define TOOLBAR_TITLE_FONT_HEIGHT 18
#define TOOLBAR_ACTION_LIMIT 4

lv_obj_t* toolbar_create(lv_obj_t* parent, const std::string& title);
lv_obj_t* toolbar_create(lv_obj_t* parent, const app::AppContext& app);
void toolbar_set_title(lv_obj_t* obj, const std::string& title);
void toolbar_set_nav_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* user_data);
lv_obj_t* toolbar_add_button_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* user_data);
lv_obj_t* toolbar_add_switch_action(lv_obj_t* obj);
lv_obj_t* toolbar_add_spinner_action(lv_obj_t* obj);

} // namespace
