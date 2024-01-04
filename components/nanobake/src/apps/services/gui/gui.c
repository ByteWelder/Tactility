#include "check.h"
#include "esp_lvgl_port.h"
#include "furi_extra_defines.h"
#include "gui_i.h"
#include "log.h"
#include "record.h"

#define TAG "gui"

// Forward declarations from gui_draw.c
bool gui_redraw_fs(Gui*);
void gui_redraw(Gui*);
static int32_t gui_main(void*);

ViewPort* gui_view_port_find_enabled(ViewPortArray_t array) {
    // Iterating backward
    ViewPortArray_it_t it;
    ViewPortArray_it_last(it, array);
    while (!ViewPortArray_end_p(it)) {
        ViewPort* view_port = *ViewPortArray_ref(it);
        if (view_port_is_enabled(view_port)) {
            ViewPort* view_port = *ViewPortArray_ref(it);
            return view_port;
        }
        ViewPortArray_previous(it);
    }
    return NULL;
}

size_t gui_active_view_port_count(Gui* gui, GuiLayer layer) {
    furi_assert(gui);
    furi_check(layer < GuiLayerMAX);
    size_t ret = 0;

    gui_lock(gui);
    ViewPortArray_it_t it;
    ViewPortArray_it_last(it, gui->layers[layer]);
    while (!ViewPortArray_end_p(it)) {
        ViewPort* view_port = *ViewPortArray_ref(it);
        if (view_port_is_enabled(view_port)) {
            ret++;
        }
        ViewPortArray_previous(it);
    }
    gui_unlock(gui);

    return ret;
}

void gui_update(Gui* gui) {
    furi_assert(gui);

    FuriThreadId thread_id = furi_thread_get_id(gui->thread);
    furi_thread_flags_set(thread_id, GUI_THREAD_FLAG_DRAW);
}

void gui_lock(Gui* gui) {
    furi_assert(gui);
    furi_check(furi_mutex_acquire(gui->mutex, FuriWaitForever) == FuriStatusOk);
}

void gui_unlock(Gui* gui) {
    furi_assert(gui);
    furi_check(furi_mutex_release(gui->mutex) == FuriStatusOk);
}

void gui_add_view_port(Gui* gui, ViewPort* view_port, GuiLayer layer) {
    furi_assert(gui);
    furi_assert(view_port);
    furi_check(layer < GuiLayerMAX);

    gui_lock(gui);
    // Verify that view port is not yet added
    ViewPortArray_it_t it;
    for (size_t i = 0; i < GuiLayerMAX; i++) {
        ViewPortArray_it(it, gui->layers[i]);
        while (!ViewPortArray_end_p(it)) {
            furi_assert(*ViewPortArray_ref(it) != view_port);
            ViewPortArray_next(it);
        }
    }
    // Add view port and link with gui
    ViewPortArray_push_back(gui->layers[layer], view_port);
    view_port_gui_set(view_port, gui);
    gui_unlock(gui);

    // Request redraw
    gui_update(gui);
}

void gui_remove_view_port(Gui* gui, ViewPort* view_port) {
    furi_assert(gui);
    furi_assert(view_port);

    gui_lock(gui);
    view_port_gui_set(view_port, NULL);
    ViewPortArray_it_t it;
    for (size_t i = 0; i < GuiLayerMAX; i++) {
        ViewPortArray_it(it, gui->layers[i]);
        while (!ViewPortArray_end_p(it)) {
            if (*ViewPortArray_ref(it) == view_port) {
                ViewPortArray_remove(gui->layers[i], it);
            } else {
                ViewPortArray_next(it);
            }
        }
    }
    /*
    if(gui->ongoing_input_view_port == view_port) {
        gui->ongoing_input_view_port = NULL;
    }
    */
    gui_unlock(gui);

    // Request redraw
    gui_update(gui);
}

void gui_view_port_send_to_front(Gui* gui, ViewPort* view_port) {
    furi_assert(gui);
    furi_assert(view_port);

    gui_lock(gui);
    // Remove
    GuiLayer layer = GuiLayerMAX;
    ViewPortArray_it_t it;
    for (size_t i = 0; i < GuiLayerMAX; i++) {
        ViewPortArray_it(it, gui->layers[i]);
        while (!ViewPortArray_end_p(it)) {
            if (*ViewPortArray_ref(it) == view_port) {
                ViewPortArray_remove(gui->layers[i], it);
                furi_assert(layer == GuiLayerMAX);
                layer = i;
            } else {
                ViewPortArray_next(it);
            }
        }
    }
    furi_assert(layer != GuiLayerMAX);
    // Return to the top
    ViewPortArray_push_back(gui->layers[layer], view_port);
    gui_unlock(gui);

    // Request redraw
    gui_update(gui);
}

void gui_view_port_send_to_back(Gui* gui, ViewPort* view_port) {
    furi_assert(gui);
    furi_assert(view_port);

    gui_lock(gui);
    // Remove
    GuiLayer layer = GuiLayerMAX;
    ViewPortArray_it_t it;
    for (size_t i = 0; i < GuiLayerMAX; i++) {
        ViewPortArray_it(it, gui->layers[i]);
        while (!ViewPortArray_end_p(it)) {
            if (*ViewPortArray_ref(it) == view_port) {
                ViewPortArray_remove(gui->layers[i], it);
                furi_assert(layer == GuiLayerMAX);
                layer = i;
            } else {
                ViewPortArray_next(it);
            }
        }
    }
    furi_assert(layer != GuiLayerMAX);
    // Return to the top
    ViewPortArray_push_at(gui->layers[layer], 0, view_port);
    gui_unlock(gui);

    // Request redraw
    gui_update(gui);
}

Gui* gui_alloc() {
    Gui* gui = malloc(sizeof(Gui));
    gui->thread = NULL;

    gui->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    furi_check(lvgl_port_lock(100));
    gui->lvgl_parent = lv_scr_act();
    lvgl_port_unlock();

    for (size_t i = 0; i < GuiLayerMAX; i++) {
        ViewPortArray_init(gui->layers[i]);
    }

    /*
    // Input
    gui->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    gui->input_events = furi_record_open(RECORD_INPUT_EVENTS);

    furi_check(gui->input_events);
    furi_pubsub_subscribe(gui->input_events, gui_input_events_callback, gui);
    */
    return gui;
}

void gui_free(Gui* gui) {
    furi_thread_free(gui->thread);

    if (gui->mutex) {
        furi_mutex_free(gui->mutex);
    }

    for (size_t i = 0; i < GuiLayerMAX; i++) {
        ViewPortArray_clear(gui->layers[i]);
    }

    free(gui);
}

static int32_t gui_main(void* p) {
    UNUSED(p);

    Gui* gui = gui_alloc();
    gui->thread = furi_thread_get_current();
    furi_record_create(RECORD_GUI, gui);

    while (1) {
        uint32_t flags = furi_thread_flags_wait(
            GUI_THREAD_FLAG_ALL,
            FuriFlagWaitAny,
            FuriWaitForever
        );
        // Process and dispatch input
        /*if (flags & GUI_THREAD_FLAG_INPUT) {
            // Process till queue become empty
            InputEvent input_event;
            while(furi_message_queue_get(gui->input_queue, &input_event, 0) == FuriStatusOk) {
                gui_input(gui, &input_event);
            }
        }*/
        // Process and dispatch draw call
        if (flags & GUI_THREAD_FLAG_DRAW) {
            FURI_LOG_D(TAG, "redraw requested");
            furi_thread_flags_clear(GUI_THREAD_FLAG_DRAW);
            gui_redraw(gui);
        }

        if (flags & GUI_THREAD_FLAG_EXIT) {
            furi_thread_flags_clear(GUI_THREAD_FLAG_EXIT);
            break;
        }
    }

    return 0;
}

static void gui_start(void* parameter) {
    UNUSED(parameter);

    FuriThread* thread = furi_thread_alloc_ex(
        "gui",
        2048,
        &gui_main,
        NULL
    );

    furi_thread_mark_as_service(thread);
    furi_thread_set_priority(thread, FuriThreadPriorityNormal);
    furi_thread_start(thread);
}

static void gui_stop() {
    FURI_RECORD_TRANSACTION(RECORD_GUI, Gui*, gui, {
        gui_lock(gui);

        FuriThreadId thread_id = furi_thread_get_id(gui->thread);
        furi_thread_flags_set(thread_id, GUI_THREAD_FLAG_EXIT);
        furi_thread_join(gui->thread);

        gui_unlock(gui);

        gui_free(gui);
    })

    furi_record_destroy(RECORD_GUI);
}

const AppManifest gui_app = {
    .id = "gui",
    .name = "GUI",
    .icon = NULL,
    .type = AppTypeService,
    .on_start = &gui_start,
    .on_stop = &gui_stop,
    .on_show = NULL,
    .stack_size = AppStackSizeNormal
};
