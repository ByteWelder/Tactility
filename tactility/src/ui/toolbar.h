#pragma once

#include "lvgl.h"
#include "app.h"

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

lv_obj_t* tt_toolbar_create(lv_obj_t* parent, const char* title);
lv_obj_t* tt_toolbar_create_for_app(lv_obj_t* parent, App app);
void tt_toolbar_set_title(lv_obj_t* obj, const char* title);
void tt_toolbar_set_nav_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* user_data);
uint8_t tt_toolbar_add_action(lv_obj_t* obj, const char* icon, const char* text, lv_event_cb_t callback, void* user_data);
