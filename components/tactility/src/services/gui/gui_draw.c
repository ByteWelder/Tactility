#include "check.h"
#include "esp_lvgl_port.h"
#include "gui_i.h"
#include "log.h"
#include "services/gui/widgets/widgets.h"
#include "services/loader/loader.h"

#define TAG "gui"

static lv_obj_t* screen_with_top_bar(lv_obj_t* parent) {
    lv_obj_set_style_bg_blacken(parent);

    lv_obj_t* vertical_container = lv_obj_create(parent);
    lv_obj_set_size(vertical_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(vertical_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_no_padding(vertical_container);
    lv_obj_set_style_bg_blacken(vertical_container);

    top_bar(vertical_container);

    lv_obj_t* child_container = lv_obj_create(vertical_container);
    lv_obj_set_width(child_container, LV_PCT(100));
    lv_obj_set_flex_grow(child_container, 1);
    lv_obj_set_style_no_padding(vertical_container);
    lv_obj_set_style_bg_blacken(vertical_container);

    return child_container;
}

static lv_obj_t* screen_with_top_bar_and_toolbar(lv_obj_t* parent) {
    lv_obj_set_style_bg_blacken(parent);

    lv_obj_t* vertical_container = lv_obj_create(parent);
    lv_obj_set_size(vertical_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(vertical_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_no_padding(vertical_container);
    lv_obj_set_style_bg_blacken(vertical_container);

    top_bar(vertical_container);

    const AppManifest* manifest = loader_get_current_app();
    if (manifest != NULL) {
        toolbar(vertical_container, TOP_BAR_HEIGHT, manifest);
    }

    lv_obj_t* spacer = lv_obj_create(vertical_container);
    lv_obj_set_size(spacer, 2, 2);
    lv_obj_set_style_bg_blacken(spacer);

    lv_obj_t* child_container = lv_obj_create(vertical_container);
    lv_obj_set_width(child_container, LV_PCT(100));
    lv_obj_set_flex_grow(child_container, 1);
    lv_obj_set_style_no_padding(vertical_container);
    lv_obj_set_style_bg_blacken(vertical_container);

    return child_container;
}

static bool gui_redraw_window(Gui* gui) {
    ViewPort* view_port = gui->layers[GuiLayerWindow];
    if (view_port) {
        lv_obj_t* container = screen_with_top_bar_and_toolbar(gui->lvgl_parent);
        view_port_draw(view_port, container);
        return true;
    } else {
        return false;
    }
}

static bool gui_redraw_desktop(Gui* gui) {
    ViewPort* view_port = gui->layers[GuiLayerDesktop];
    if (view_port) {
        lv_obj_t* container = screen_with_top_bar(gui->lvgl_parent);
        view_port_draw(view_port, container);
        return true;
    } else {
        FURI_LOG_E(TAG, "no desktop layer found");
    }

    return false;
}

bool gui_redraw_fs(Gui* gui) {
    ViewPort* view_port = gui->layers[GuiLayerFullscreen];
    if (view_port) {
        view_port_draw(view_port, gui->lvgl_parent);
        return true;
    } else {
        return false;
    }
}

void gui_redraw(Gui* gui) {
    furi_assert(gui);

    furi_check(lvgl_port_lock(100));
    lv_obj_clean(gui->lvgl_parent);

    if (!gui_redraw_fs(gui)) {
        if (!gui_redraw_window(gui)) {
            gui_redraw_desktop(gui);
        }
    }

    lvgl_port_unlock();
}
