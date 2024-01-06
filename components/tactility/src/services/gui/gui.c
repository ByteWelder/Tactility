#include "check.h"
#include "esp_lvgl_port.h"
#include "furi_extra_defines.h"
#include "gui_i.h"
#include "log.h"
#include "kernel.h"

#define TAG "gui"

// Forward declarations
bool gui_redraw_fs(Gui*);
void gui_redraw(Gui*);
static int32_t gui_main(void*);

static Gui* gui = NULL;

Gui* gui_alloc() {
    Gui* instance = malloc(sizeof(Gui));
    memset(instance, 0, sizeof(Gui));
    furi_check(instance != NULL);
    instance->thread = furi_thread_alloc_ex(
        "gui",
        4096, // Last known minimum was 2800 for launching desktop
        &gui_main,
        NULL
    );
    instance->active_layer = GuiLayerNone;
    instance->mutex = furi_mutex_alloc(FuriMutexTypeRecursive);

    furi_check(lvgl_port_lock(100));
    instance->lvgl_parent = lv_scr_act();
    lvgl_port_unlock();

    for (size_t i = 0; i < GuiLayerMAX; i++) {
        instance->layers[i] = NULL;
    }

    /*
    // Input
    gui->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    gui->input_events = furi_record_open(RECORD_INPUT_EVENTS);

    furi_check(gui->input_events);
    furi_pubsub_subscribe(gui->input_events, gui_input_events_callback, gui);
    */
    return instance;
}

void gui_free(Gui* instance) {
    furi_assert(instance != NULL);
    furi_thread_free(instance->thread);
    furi_mutex_free(instance->mutex);
    free(instance);
}

void gui_lock() {
    furi_assert(gui);
    furi_assert(gui->mutex);
    furi_check(furi_mutex_acquire(gui->mutex, 1000 / portTICK_PERIOD_MS) == FuriStatusOk);
}

void gui_unlock() {
    furi_assert(gui);
    furi_assert(gui->mutex);
    furi_check(furi_mutex_release(gui->mutex) == FuriStatusOk);
}

void gui_request_draw() {
    furi_assert(gui);

    FuriThreadId thread_id = furi_thread_get_id(gui->thread);
    furi_thread_flags_set(thread_id, GUI_THREAD_FLAG_DRAW);
}

void gui_add_view_port(ViewPort* view_port, GuiLayer layer) {
    furi_assert(gui);
    furi_assert(view_port);
    furi_check(layer < GuiLayerMAX);

    gui_lock();
    furi_check(gui->layers[layer] == NULL, "layer in use");
    gui->layers[layer] = view_port;
    view_port_gui_set(view_port, gui);
    gui_unlock();

    gui_request_draw();
}

void gui_remove_view_port(ViewPort* view_port) {
    furi_assert(gui);
    furi_assert(view_port);

    gui_lock();

    view_port_gui_set(view_port, NULL);
    for (size_t i = 0; i < GuiLayerMAX; i++) {
        if (gui->layers[i] == view_port) {
            gui->layers[i] = NULL;
            break;
        }
    }

    gui_unlock();

    gui_request_draw();
}

static int32_t gui_main(void* p) {
    UNUSED(p);
    furi_check(gui);
    Gui* local_gui = gui;

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
            furi_thread_flags_clear(GUI_THREAD_FLAG_DRAW);
            gui_redraw(local_gui);
        }

        if (flags & GUI_THREAD_FLAG_EXIT) {
            furi_thread_flags_clear(GUI_THREAD_FLAG_EXIT);
            break;
        }
    }

    return 0;
}

// region AppManifest

static void gui_start(Context* context) {
    UNUSED(context);

    gui = gui_alloc();

    furi_thread_set_priority(gui->thread, FuriThreadPriorityNormal);
    furi_thread_start(gui->thread);
}

static void gui_stop(Context* context) {
    UNUSED(context);

    gui_lock();

    FuriThreadId thread_id = furi_thread_get_id(gui->thread);
    furi_thread_flags_set(thread_id, GUI_THREAD_FLAG_EXIT);
    furi_thread_join(gui->thread);

    gui_unlock();

    gui_free(gui);
}

const ServiceManifest gui_service = {
    .id = "gui",
    .on_start = &gui_start,
    .on_stop = &gui_stop
};

// endregion
