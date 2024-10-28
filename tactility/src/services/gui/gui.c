#include "gui_i.h"

#include "tactility.h"
#include "services/loader/loader.h"
#include "ui/lvgl_sync.h"
#include "ui/lvgl_keypad.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#else
#include "FreeRTOS.h"
#endif

#define TAG "gui"

// Forward declarations
void gui_redraw(Gui*);
static int32_t gui_main(void*);

Gui* gui = NULL;

typedef void (*PubSubCallback)(const void* message, void* context);
void gui_loader_callback(const void* message, void* context) {
    Gui* gui = (Gui*)context;
    LoaderEvent* event = (LoaderEvent*)message;
    if (event->type == LoaderEventTypeApplicationShowing) {
        App* app = event->app_showing.app;
        const AppManifest* app_manifest = tt_app_get_manifest(app);
        gui_show_app(app, app_manifest->on_show, app_manifest->on_hide);
    } else if (event->type == LoaderEventTypeApplicationHiding) {
        gui_hide_app();
    }
}

Gui* gui_alloc() {
    Gui* instance = malloc(sizeof(Gui));
    memset(instance, 0, sizeof(Gui));
    tt_check(instance != NULL);
    instance->thread = tt_thread_alloc_ex(
        "gui",
        4096, // Last known minimum was 2800 for launching desktop
        &gui_main,
        NULL
    );
    instance->mutex = tt_mutex_alloc(MutexTypeRecursive);
    instance->keyboard = NULL;
    instance->loader_pubsub_subscription = tt_pubsub_subscribe(loader_get_pubsub(), &gui_loader_callback, instance);
    tt_check(tt_lvgl_lock(1000 / portTICK_PERIOD_MS));
    instance->keyboard_group = lv_group_create();
    instance->lvgl_parent = lv_scr_act();
    tt_lvgl_unlock();

    return instance;
}

void gui_free(Gui* instance) {
    tt_assert(instance != NULL);
    tt_thread_free(instance->thread);
    tt_mutex_free(instance->mutex);

    tt_check(tt_lvgl_lock(1000 / portTICK_PERIOD_MS));
    lv_group_del(instance->keyboard_group);
    tt_lvgl_unlock();

    free(instance);
}

void gui_lock() {
    tt_assert(gui);
    tt_assert(gui->mutex);
    tt_check(tt_mutex_acquire(gui->mutex, configTICK_RATE_HZ) == TtStatusOk);
}

void gui_unlock() {
    tt_assert(gui);
    tt_assert(gui->mutex);
    tt_check(tt_mutex_release(gui->mutex) == TtStatusOk);
}

void gui_request_draw() {
    tt_assert(gui);
    ThreadId thread_id = tt_thread_get_id(gui->thread);
    tt_thread_flags_set(thread_id, GUI_THREAD_FLAG_DRAW);
}

void gui_show_app(App app, ViewPortShowCallback on_show, ViewPortHideCallback on_hide) {
    gui_lock();
    tt_check(gui->app_view_port == NULL);
    gui->app_view_port = view_port_alloc(app, on_show, on_hide);
    gui_unlock();
    gui_request_draw();
}

void gui_hide_app() {
    gui_lock();
    ViewPort* view_port = gui->app_view_port;
    tt_check(view_port != NULL);

    // We must lock the LVGL port, because the viewport hide callbacks
    // might call LVGL APIs (e.g. to remove the keyboard from the screen root)
    tt_check(tt_lvgl_lock(configTICK_RATE_HZ));
    view_port_hide(view_port);
    tt_lvgl_unlock();

    view_port_free(view_port);
    gui->app_view_port = NULL;
    gui_unlock();
}

static int32_t gui_main(TT_UNUSED void* p) {
    tt_check(gui);
    Gui* local_gui = gui;

    while (1) {
        uint32_t flags = tt_thread_flags_wait(
            GUI_THREAD_FLAG_ALL,
            TtFlagWaitAny,
            TtWaitForever
        );
        // Process and dispatch draw call
        if (flags & GUI_THREAD_FLAG_DRAW) {
            tt_thread_flags_clear(GUI_THREAD_FLAG_DRAW);
            gui_redraw(local_gui);
        }

        if (flags & GUI_THREAD_FLAG_EXIT) {
            tt_thread_flags_clear(GUI_THREAD_FLAG_EXIT);
            break;
        }
    }

    return 0;
}

// region AppManifest

static void gui_start(TT_UNUSED Service service) {
    gui = gui_alloc();

    tt_thread_set_priority(gui->thread, THREAD_PRIORITY_SERVICE);
    tt_thread_start(gui->thread);
}

static void gui_stop(TT_UNUSED Service service) {
    gui_lock();

    ThreadId thread_id = tt_thread_get_id(gui->thread);
    tt_thread_flags_set(thread_id, GUI_THREAD_FLAG_EXIT);
    tt_thread_join(gui->thread);
    tt_thread_free(gui->thread);

    gui_unlock();

    gui_free(gui);
}

const ServiceManifest gui_service = {
    .id = "gui",
    .on_start = &gui_start,
    .on_stop = &gui_stop
};

// endregion
