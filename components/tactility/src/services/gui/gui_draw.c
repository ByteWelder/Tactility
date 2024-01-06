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

static GuiLayer gui_get_desired_layer(Gui* gui) {
    // No need to lock, as this is done by gui_redraw()
    if (gui->layers[GuiLayerFullscreen] != NULL) {
        return GuiLayerFullscreen;
    } else if (gui->layers[GuiLayerWindow] != NULL) {
        return GuiLayerWindow;
    } else if (gui->layers[GuiLayerDesktop] != NULL) {
        return GuiLayerDesktop;
    } else {
        furi_crash("no gui layer active");
    }
}

static void gui_redraw_window(Gui* gui) {
    // No need to lock, as this is done by gui_redraw()
    ViewPort* view_port = gui->layers[GuiLayerWindow];
    furi_assert(view_port);
    lv_obj_t* container = screen_with_top_bar_and_toolbar(gui->lvgl_parent);
    view_port_draw(view_port, container);
}

static void gui_redraw_desktop(Gui* gui) {
    // No need to lock, as this is done by gui_redraw()
    ViewPort* view_port = gui->layers[GuiLayerDesktop];
    furi_assert(view_port);
    lv_obj_t* container = screen_with_top_bar(gui->lvgl_parent);
    view_port_draw(view_port, container);
}

static void gui_redraw_fs(Gui* gui) {
    // No need to lock, as this is done by gui_redraw()
    ViewPort* view_port = gui->layers[GuiLayerFullscreen];
    furi_assert(view_port);
    view_port_draw(view_port, gui->lvgl_parent);
}

void gui_redraw(Gui* gui) {
    furi_assert(gui);

    // Lock GUI and LVGL
    gui_lock();
    furi_check(lvgl_port_lock(100));

    lv_obj_clean(gui->lvgl_parent);

    if (gui->active_layer != GuiLayerNone) {
        ViewPort* view_port = gui->layers[gui->active_layer];
        furi_assert(view_port);
        view_port_undraw(view_port);
    }

    lv_obj_clean(gui->lvgl_parent);

    GuiLayer new_layer = gui_get_desired_layer(gui);
    switch (new_layer) {
        case GuiLayerFullscreen:
            gui_redraw_fs(gui);
            break;
        case GuiLayerWindow:
            gui_redraw_window(gui);
            break;
        case GuiLayerDesktop:
            gui_redraw_desktop(gui);
            break;
        default:
            break;
    }

    // Unlock GUI and LVGL
    lvgl_port_unlock();
    gui_unlock();
}
