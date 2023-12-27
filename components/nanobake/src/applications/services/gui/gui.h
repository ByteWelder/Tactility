#pragma once

#include "nb_app.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_GUI "gui"

typedef uint16_t ScreenId;

typedef struct NbGui* NbGuiHandle;
typedef void (*InitScreen)(lv_obj_t*, ScreenId);

ScreenId gui_screen_create(NbGuiHandle _Nonnull gui, InitScreen callback);
void gui_screen_free(NbGuiHandle _Nonnull gui, ScreenId id);
// TODO make internal
void gui_screen_set_parent(NbGuiHandle _Nonnull gui, ScreenId id, lv_obj_t* parent);
lv_obj_t* gui_screen_get_parent(NbGuiHandle _Nonnull gui, ScreenId id);

extern const NbApp gui_app;

#ifdef __cplusplus
}
#endif
