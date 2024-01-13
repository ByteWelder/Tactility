#include "check.h"
#include "esp_lvgl_port.h"
#include "gui_i.h"
#include "log.h"
#include "services/gui/widgets/statusbar.h"
#include "services/loader/loader.h"
#include "ui/spacer.h"
#include "ui/style.h"
#include "ui/toolbar.h"

#define TAG "gui"

static lv_obj_t* create_app_views(lv_obj_t* parent, App app) {
    tt_lv_obj_set_style_bg_blacken(parent);

    lv_obj_t* vertical_container = lv_obj_create(parent);
    lv_obj_set_size(vertical_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(vertical_container, LV_FLEX_FLOW_COLUMN);
    tt_lv_obj_set_style_no_padding(vertical_container);
    tt_lv_obj_set_style_bg_blacken(vertical_container);

    // TODO: Move statusbar into separate ViewPort
    AppFlags flags = app_get_flags(app);
    if (flags.show_statusbar) {
        tt_lv_statusbar_create(vertical_container);
    }

    if (flags.show_toolbar) {
        const AppManifest* manifest = app_get_manifest(app);
        if (manifest != NULL) {
            // TODO: Keep toolbar on app level so app can update it (app_set_toolbar() etc?)
            Toolbar toolbar = {
                .nav_action = &loader_stop_app,
                .nav_icon = LV_SYMBOL_CLOSE,
                .title = manifest->name
            };
            lv_obj_t* toolbar_widget = tt_lv_toolbar_create(vertical_container, &toolbar);
            lv_obj_set_pos(toolbar_widget, 0, STATUSBAR_HEIGHT);

            // Black area between toolbar and content below
            lv_obj_t* spacer = tt_lv_spacer_create(vertical_container, 1, 2);
            tt_lv_obj_set_style_bg_blacken(spacer);
        }
    }

    lv_obj_t* child_container = lv_obj_create(vertical_container);
    lv_obj_set_width(child_container, LV_PCT(100));
    lv_obj_set_flex_grow(child_container, 1);

    return child_container;
}

void gui_redraw(Gui* gui) {
    furi_assert(gui);

    // Lock GUI and LVGL
    gui_lock();
    furi_check(lvgl_port_lock(100));

    lv_obj_clean(gui->lvgl_parent);

    if (gui->app_view_port != NULL) {
        ViewPort* view_port = gui->app_view_port;
        furi_assert(view_port);
        App app = gui->app_view_port->app;
        lv_obj_t* container = create_app_views(gui->lvgl_parent, app);
        view_port_show(view_port, container);
    } else {
        FURI_LOG_W(TAG, "nothing to draw");
    }

    // Unlock GUI and LVGL
    lvgl_port_unlock();
    gui_unlock();
}
