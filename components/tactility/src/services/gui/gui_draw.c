#include "check.h"
#include "esp_lvgl_port.h"
#include "gui_i.h"
#include "log.h"
#include "services/gui/widgets/statusbar.h"
#include "services/gui/widgets/toolbar.h"
#include "services/loader/loader.h"
#include "ui/style.h"

#define TAG "gui"

static lv_obj_t* create_app_views(lv_obj_t* parent, AppFlags flags) {
    // TODO: Move statusbar into separate ViewPort?
    // TODO: Move toolbar into separate ViewPort?

    tt_lv_obj_set_style_bg_blacken(parent);

    lv_obj_t* vertical_container = lv_obj_create(parent);
    lv_obj_set_size(vertical_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(vertical_container, LV_FLEX_FLOW_COLUMN);
    tt_lv_obj_set_style_no_padding(vertical_container);
    tt_lv_obj_set_style_bg_blacken(vertical_container);

    if (flags.show_statusbar) {
        tt_lv_statusbar_create(vertical_container);
    }

    if (flags.show_toolbar) {
        // TODO: Make some kind of Toolbar struct to hold the title and back icon
        const AppManifest* manifest = loader_get_current_app();
        if (manifest != NULL) {
            tt_lv_toolbar_create(vertical_container, STATUSBAR_HEIGHT, manifest);

            lv_obj_t* spacer = lv_obj_create(vertical_container);
            lv_obj_set_size(spacer, 2, 2);
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
        lv_obj_t* container = create_app_views(gui->lvgl_parent, gui->app_flags);
        view_port_show(view_port, container);
    } else {
        FURI_LOG_W(TAG, "nothing to draw");
    }

    // Unlock GUI and LVGL
    lvgl_port_unlock();
    gui_unlock();
}
