#include "check.h"
#include "gui.h"
#include "gui_i.h"
#include "view_port_i.h"

#define TAG "viewport"

_Static_assert(ViewPortOrientationMAX == 4, "Incorrect ViewPortOrientation count");
_Static_assert(
    (ViewPortOrientationHorizontal == 0 && ViewPortOrientationHorizontalFlip == 1 &&
     ViewPortOrientationVertical == 2 && ViewPortOrientationVerticalFlip == 3),
    "Incorrect ViewPortOrientation order"
);

ViewPort* view_port_alloc() {
    ViewPort* view_port = malloc(sizeof(ViewPort));
    view_port->gui = NULL;
    view_port->is_enabled = true;
    view_port->mutex = furi_mutex_alloc(FuriMutexTypeRecursive);
    return view_port;
}

void view_port_free(ViewPort* view_port) {
    furi_assert(view_port);
    furi_check(furi_mutex_acquire(view_port->mutex, FuriWaitForever) == FuriStatusOk);
    furi_check(view_port->gui == NULL);
    furi_check(furi_mutex_release(view_port->mutex) == FuriStatusOk);
    furi_mutex_free(view_port->mutex);
    free(view_port);
}

void view_port_enabled_set(ViewPort* view_port, bool enabled) {
    furi_assert(view_port);
    furi_check(furi_mutex_acquire(view_port->mutex, FuriWaitForever) == FuriStatusOk);
    if (view_port->is_enabled != enabled) {
        view_port->is_enabled = enabled;
        if (view_port->gui) gui_update(view_port->gui);
    }
    furi_check(furi_mutex_release(view_port->mutex) == FuriStatusOk);
}

bool view_port_is_enabled(const ViewPort* view_port) {
    furi_assert(view_port);
    furi_check(furi_mutex_acquire(view_port->mutex, FuriWaitForever) == FuriStatusOk);
    bool is_enabled = view_port->is_enabled;
    furi_check(furi_mutex_release(view_port->mutex) == FuriStatusOk);
    return is_enabled;
}

void view_port_draw_callback_set(ViewPort* view_port, ViewPortDrawCallback callback, void* context) {
    furi_assert(view_port);
    furi_check(furi_mutex_acquire(view_port->mutex, FuriWaitForever) == FuriStatusOk);
    view_port->draw_callback = callback;
    view_port->draw_callback_context = context;
    furi_check(furi_mutex_release(view_port->mutex) == FuriStatusOk);
}

void view_port_update(ViewPort* view_port) {
    furi_assert(view_port);

    // We are not going to lockup system, but will notify you instead
    // Make sure that you don't call viewport methods inside another mutex, especially one that is used in draw call
    if (furi_mutex_acquire(view_port->mutex, 2) != FuriStatusOk) {
        ESP_LOGW(TAG, "ViewPort lockup: see %s:%d", __FILE__, __LINE__ - 3);
    }

    if (view_port->gui && view_port->is_enabled) gui_update(view_port->gui);
    furi_mutex_release(view_port->mutex);
}

void view_port_gui_set(ViewPort* view_port, Gui* gui) {
    furi_assert(view_port);
    furi_check(furi_mutex_acquire(view_port->mutex, FuriWaitForever) == FuriStatusOk);
    view_port->gui = gui;
    furi_check(furi_mutex_release(view_port->mutex) == FuriStatusOk);
}

void view_port_draw(ViewPort* view_port, lv_obj_t* parent) {
    furi_assert(view_port);
    furi_assert(parent);

    // We are not going to lockup system, but will notify you instead
    // Make sure that you don't call viewport methods inside another mutex, especially one that is used in draw call
    if (furi_mutex_acquire(view_port->mutex, 2) != FuriStatusOk) {
        ESP_LOGW(TAG, "ViewPort lockup: see %s:%d", __FILE__, __LINE__ - 3);
    }

    furi_check(view_port->gui);

    if (view_port->draw_callback) {
        lv_obj_clean(parent);
        view_port->draw_callback(parent, view_port->draw_callback_context);
    }

    furi_mutex_release(view_port->mutex);
}
