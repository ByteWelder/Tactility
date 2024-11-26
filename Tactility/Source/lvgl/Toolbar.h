#pragma once

#include "lvgl.h"
#include "app/App.h"

namespace tt::lvgl {

#define TOOLBAR_HEIGHT 40
#define TOOLBAR_ACTION_LIMIT 8
#define TOOLBAR_TITLE_FONT_HEIGHT 18

typedef void(*ToolbarActionCallback)(void* _Nullable context);

typedef struct {
    const char* icon;
    const char* text;
    ToolbarActionCallback callback;
    void* _Nullable callback_context;
} ToolbarAction;

lv_obj_t* toolbar_create(lv_obj_t* parent, const std::string& title);
lv_obj_t* toolbar_create(lv_obj_t* parent, const app::App& app);
void toolbar_set_title(lv_obj_t* obj, const std::string& title);
void toolbar_set_nav_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* user_data);
uint8_t toolbar_add_action(lv_obj_t* obj, const char* icon, const char* text, lv_event_cb_t callback, void* user_data);

} // namespace
