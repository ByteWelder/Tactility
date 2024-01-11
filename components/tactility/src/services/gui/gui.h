#pragma once

#include "service_manifest.h"
#include "view_port.h"
#include "app_i.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Gui Gui;

void gui_show_app(Context* context, ViewPortShowCallback on_show, ViewPortHideCallback on_hide, AppFlags flags);

void gui_hide_app();

#ifdef __cplusplus
}
#endif
