#pragma once

#include "lvgl.h"
#include "app.h"

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct {
    lv_obj_t obj;
    lv_obj_t* title_label;
    lv_obj_t* close_button;
    lv_obj_t* close_button_image;
    lv_obj_t* action_container;
    ToolbarAction* action_array[TOOLBAR_ACTION_LIMIT];
    uint8_t  action_count;
} Toolbar;

lv_obj_t* tt_toolbar_create(lv_obj_t* parent, const char* title);
lv_obj_t* tt_toolbar_create_for_app(lv_obj_t* parent, App app);
void tt_toolbar_set_title(lv_obj_t* obj, const char* title);
void tt_toolbar_set_nav_action(lv_obj_t* obj, const char* icon, lv_event_cb_t callback, void* user_data);
uint8_t tt_toolbar_add_action(lv_obj_t* obj, const char* icon, const char* text, lv_event_cb_t callback, void* user_data);

#ifdef __cplusplus
}
#endif
