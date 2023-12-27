#pragma once

#include "nb_app.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_GUI "gui"

typedef uint16_t screen_id_t;

typedef struct Gui Gui;
typedef void (*on_init_lvgl)(lv_obj_t*, screen_id_t);

screen_id_t gui_screen_create(Gui* gui, on_init_lvgl callback);
void gui_screen_free(Gui* gui, screen_id_t id);
// TODO make internal
void gui_screen_set_parent(Gui* gui, screen_id_t id, lv_obj_t* parent);
lv_obj_t* gui_screen_get_parent(Gui* gui, screen_id_t id);

extern const nb_app_t gui_app;

#ifdef __cplusplus
}
#endif
