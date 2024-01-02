#include "check.h"
#include "gui_i.h"
#include "esp_lvgl_port.h"
#include "apps/services/gui/widgets/widgets.h"

static void gui_redraw_status_bar(Gui* gui, bool need_attention) {
    /*
    ViewPortArray_it_t it;
    uint8_t left_used = 0;
    uint8_t right_used = 0;
    uint8_t width;

    canvas_frame_set(
        gui->lvgl_parent, GUI_STATUS_BAR_X, GUI_STATUS_BAR_Y, GUI_DISPLAY_WIDTH, GUI_STATUS_BAR_HEIGHT);

    // for support black theme - paint white area and
    // draw icon with transparent white color

    canvas_set_color(gui->canvas, ColorWhite);
    canvas_draw_box(gui->canvas, 1, 1, 9, 7);
    canvas_draw_box(gui->canvas, 7, 3, 58, 6);
    canvas_draw_box(gui->canvas, 61, 1, 32, 7);
    canvas_draw_box(gui->canvas, 89, 3, 38, 6);
    canvas_set_color(gui->canvas, ColorBlack);
    canvas_set_bitmap_mode(gui->canvas, 1);
    canvas_draw_icon(gui->canvas, 0, 0, &I_Background_128x11);
    canvas_set_bitmap_mode(gui->canvas, 0);

    // Right side
    uint8_t x = GUI_DISPLAY_WIDTH - 1;
    ViewPortArray_it(it, gui->layers[GuiLayerStatusBarRight]);
    while(!ViewPortArray_end_p(it) && right_used < GUI_STATUS_BAR_WIDTH) {
        ViewPort* view_port = *ViewPortArray_ref(it);
        if(view_port_is_enabled(view_port)) {
            width = view_port_get_width(view_port);
            if(!width) width = 8;
            // Recalculate next position
            right_used += (width + 2);
            x -= (width + 2);
            // Prepare work area background
            canvas_frame_set(
                gui->canvas,
                x - 1,
                GUI_STATUS_BAR_Y + 1,
                width + 2,
                GUI_STATUS_BAR_WORKAREA_HEIGHT + 2);
            canvas_set_color(gui->canvas, ColorWhite);
            canvas_draw_box(
                gui->canvas, 0, 0, canvas_width(gui->canvas), canvas_height(gui->canvas));
            canvas_set_color(gui->canvas, ColorBlack);
            // ViewPort draw
            canvas_frame_set(
                gui->canvas, x, GUI_STATUS_BAR_Y + 2, width, GUI_STATUS_BAR_WORKAREA_HEIGHT);
            view_port_draw(view_port, gui->canvas);
        }
        ViewPortArray_next(it);
    }
    // Draw frame around icons on the right
    if(right_used) {
        canvas_frame_set(
            gui->canvas,
            GUI_DISPLAY_WIDTH - 3 - right_used,
            GUI_STATUS_BAR_Y,
            right_used + 3,
            GUI_STATUS_BAR_HEIGHT);
        canvas_set_color(gui->canvas, ColorBlack);
        canvas_draw_rframe(
            gui->canvas, 0, 0, canvas_width(gui->canvas), canvas_height(gui->canvas), 1);
        canvas_draw_line(
            gui->canvas,
            canvas_width(gui->canvas) - 2,
            1,
            canvas_width(gui->canvas) - 2,
            canvas_height(gui->canvas) - 2);
        canvas_draw_line(
            gui->canvas,
            1,
            canvas_height(gui->canvas) - 2,
            canvas_width(gui->canvas) - 2,
            canvas_height(gui->canvas) - 2);
    }

    // Left side
    x = 2;
    ViewPortArray_it(it, gui->layers[GuiLayerStatusBarLeft]);
    while(!ViewPortArray_end_p(it) && (right_used + left_used) < GUI_STATUS_BAR_WIDTH) {
        ViewPort* view_port = *ViewPortArray_ref(it);
        if(view_port_is_enabled(view_port)) {
            width = view_port_get_width(view_port);
            if(!width) width = 8;
            // Prepare work area background
            canvas_frame_set(
                gui->canvas,
                x - 1,
                GUI_STATUS_BAR_Y + 1,
                width + 2,
                GUI_STATUS_BAR_WORKAREA_HEIGHT + 2);
            canvas_set_color(gui->canvas, ColorWhite);
            canvas_draw_box(
                gui->canvas, 0, 0, canvas_width(gui->canvas), canvas_height(gui->canvas));
            canvas_set_color(gui->canvas, ColorBlack);
            // ViewPort draw
            canvas_frame_set(
                gui->canvas, x, GUI_STATUS_BAR_Y + 2, width, GUI_STATUS_BAR_WORKAREA_HEIGHT);
            view_port_draw(view_port, gui->canvas);
            // Recalculate next position
            left_used += (width + 2);
            x += (width + 2);
        }
        ViewPortArray_next(it);
    }
    // Extra notification
    if(need_attention) {
        width = icon_get_width(&I_Hidden_window_9x8);
        // Prepare work area background
        canvas_frame_set(
            gui->canvas,
            x - 1,
            GUI_STATUS_BAR_Y + 1,
            width + 2,
            GUI_STATUS_BAR_WORKAREA_HEIGHT + 2);
        canvas_set_color(gui->canvas, ColorWhite);
        canvas_draw_box(gui->canvas, 0, 0, canvas_width(gui->canvas), canvas_height(gui->canvas));
        canvas_set_color(gui->canvas, ColorBlack);
        // Draw Icon
        canvas_frame_set(
            gui->canvas, x, GUI_STATUS_BAR_Y + 2, width, GUI_STATUS_BAR_WORKAREA_HEIGHT);
        canvas_draw_icon(gui->canvas, 0, 0, &I_Hidden_window_9x8);
        // Recalculate next position
        left_used += (width + 2);
        x += (width + 2);
    }
    // Draw frame around icons on the left
    if(left_used) {
        canvas_frame_set(gui->canvas, 0, 0, left_used + 3, GUI_STATUS_BAR_HEIGHT);
        canvas_draw_rframe(
            gui->canvas, 0, 0, canvas_width(gui->canvas), canvas_height(gui->canvas), 1);
        canvas_draw_line(
            gui->canvas,
            canvas_width(gui->canvas) - 2,
            1,
            canvas_width(gui->canvas) - 2,
            canvas_height(gui->canvas) - 2);
        canvas_draw_line(
            gui->canvas,
            1,
            canvas_height(gui->canvas) - 2,
            canvas_width(gui->canvas) - 2,
            canvas_height(gui->canvas) - 2);
    }
    */
}

static bool gui_redraw_window(Gui* gui) {
    ViewPort* view_port = gui_view_port_find_enabled(gui->layers[GuiLayerWindow]);
    if (view_port) {
        lv_obj_set_style_bg_blacken(gui->lvgl_parent);

        lv_obj_t* vertical_container = lv_obj_create(gui->lvgl_parent);
        lv_obj_set_size(vertical_container, LV_PCT(100), LV_PCT(100));
        lv_obj_set_flex_flow(vertical_container, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_no_padding(vertical_container);
        lv_obj_set_style_bg_blacken(vertical_container);

        top_bar(vertical_container);

        lv_obj_t* window_parent = lv_obj_create(vertical_container);
        lv_obj_set_width(window_parent, LV_PCT(100));
        lv_obj_set_flex_grow(window_parent, 1);
        lv_obj_set_style_no_padding(vertical_container);
        lv_obj_set_style_bg_blacken(vertical_container);

        view_port_draw(view_port, window_parent);
        return true;
    } else {
        return false;
    }
}

static bool gui_redraw_desktop(Gui* gui) {
    /*
    canvas_frame_set(gui->lvgl_parent, 0, 0, GUI_DISPLAY_WIDTH, GUI_DISPLAY_HEIGHT);
    ViewPort* view_port = gui_view_port_find_enabled(gui->layers[GuiLayerDesktop]);
    if(view_port) {
        view_port_draw(view_port, gui->lvgl_parent);
        return true;
    }
    */

    return false;
}

bool gui_redraw_fs(Gui* gui) {
    ViewPort* view_port = gui_view_port_find_enabled(gui->layers[GuiLayerFullscreen]);
    if (view_port) {
        view_port_draw(view_port, gui->lvgl_parent);
        return true;
    } else {
        return false;
    }
}

void gui_redraw(Gui* gui) {
    furi_assert(gui);
    gui_lock(gui);

    furi_check(lvgl_port_lock(100));
    lv_obj_clean(gui->lvgl_parent);

    gui_redraw_desktop(gui);
    if (!gui_redraw_fs(gui)) {
        if (!gui_redraw_window(gui)) {
            gui_redraw_desktop(gui);
        }
        gui_redraw_status_bar(gui, false);
    }

    lvgl_port_unlock();

    gui_unlock(gui);
}
