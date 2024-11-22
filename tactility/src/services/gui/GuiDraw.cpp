#include "Check.h"
#include "Log.h"
#include "gui_i_.h"
#include "ui/lvgl_sync.h"
#include "ui/statusbar.h"
#include "ui/style.h"

namespace tt::service::gui {

#define TAG "gui"

static lv_obj_t* create_app_views(Gui* gui, lv_obj_t* parent, App app) {
    lvgl::obj_set_style_bg_blacken(parent);

    lv_obj_t* vertical_container = lv_obj_create(parent);
    lv_obj_set_size(vertical_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(vertical_container, LV_FLEX_FLOW_COLUMN);
    lvgl::obj_set_style_no_padding(vertical_container);
    lvgl::obj_set_style_bg_blacken(vertical_container);

    // TODO: Move statusbar into separate ViewPort
    AppFlags flags = tt_app_get_flags(app);
    if (flags.show_statusbar) {
        lvgl::statusbar_create(vertical_container);
    }

    lv_obj_t* child_container = lv_obj_create(vertical_container);
    lv_obj_set_width(child_container, LV_PCT(100));
    lv_obj_set_flex_grow(child_container, 1);

    if (keyboard_is_enabled()) {
        gui->keyboard = lv_keyboard_create(vertical_container);
        lv_obj_add_flag(gui->keyboard, LV_OBJ_FLAG_HIDDEN);
    } else {
        gui->keyboard = nullptr;
    }

    return child_container;
}

void redraw(Gui* gui) {
    tt_assert(gui);

    // Lock GUI and LVGL
    lock();

    if (lvgl::lock(1000)) {
        lv_obj_clean(gui->lvgl_parent);

        ViewPort* view_port = gui->app_view_port;
        if (view_port != nullptr) {
            App app = view_port->app;
            lv_obj_t* container = create_app_views(gui, gui->lvgl_parent, app);
            view_port_show(view_port, container);
        } else {
            TT_LOG_W(TAG, "nothing to draw");
        }

        // Unlock GUI and LVGL
        lvgl::unlock();
    } else {
        TT_LOG_E(TAG, "failed to lock lvgl");
    }

    unlock();
}

} // namespace
