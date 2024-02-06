#include "check.h"
#include "ui/lvgl_sync.h"
#include "gui_i.h"
#include "log.h"
#include "services/gui/widgets/statusbar.h"
#include "ui/style.h"
#include "ui/toolbar.h"

#define TAG "gui"

static lv_obj_t* create_app_views(Gui* gui, lv_obj_t* parent, App app) {
    tt_lv_obj_set_style_bg_blacken(parent);

    lv_obj_t* vertical_container = lv_obj_create(parent);
    lv_obj_set_size(vertical_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(vertical_container, LV_FLEX_FLOW_COLUMN);
    tt_lv_obj_set_style_no_padding(vertical_container);
    tt_lv_obj_set_style_bg_blacken(vertical_container);

    // TODO: Move statusbar into separate ViewPort
    AppFlags flags = tt_app_get_flags(app);
    if (flags.show_statusbar) {
        tt_lv_statusbar_create(vertical_container);
    }

    lv_obj_t* child_container = lv_obj_create(vertical_container);
    lv_obj_set_width(child_container, LV_PCT(100));
    lv_obj_set_flex_grow(child_container, 1);

    if (gui_keyboard_is_enabled()) {
        gui->keyboard = lv_keyboard_create(vertical_container);
        lv_obj_add_flag(gui->keyboard, LV_OBJ_FLAG_HIDDEN);
    } else {
        gui->keyboard = NULL;
    }

    return child_container;
}

void gui_redraw(Gui* gui) {
    tt_assert(gui);

    // Lock GUI and LVGL
    gui_lock();

    if (tt_lvgl_lock(1000)) {
        lv_obj_clean(gui->lvgl_parent);

        ViewPort* view_port = gui->app_view_port;
        if (view_port != NULL) {
            App app = view_port->app;
            lv_obj_t* container = create_app_views(gui, gui->lvgl_parent, app);
            view_port_show(view_port, container);
        } else {
            TT_LOG_W(TAG, "nothing to draw");
        }

        // Unlock GUI and LVGL
        tt_lvgl_unlock();
    } else {
        TT_LOG_E(TAG, "failed to lock lvgl");
    }

    gui_unlock();
}
