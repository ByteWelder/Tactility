#include "Gui_i.h"

#include "Tactility.h"
#include "services/loader/Loader.h"
#include "ui/lvgl_keypad.h"
#include "ui/lvgl_sync.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#else
#include "FreeRTOS.h"
#endif

namespace tt::service::gui {

#define TAG "gui"

// Forward declarations
void redraw(Gui*);
static int32_t gui_main(void*);

Gui* gui = nullptr;

typedef void (*PubSubCallback)(const void* message, void* context);
void loader_callback(const void* message, TT_UNUSED void* context) {
    auto* event = static_cast<const loader::LoaderEvent*>(message);
    if (event->type == loader::LoaderEventTypeApplicationShowing) {
        App* app = event->app_showing.app;
        const AppManifest& app_manifest = tt_app_get_manifest(app);
        show_app(app, app_manifest.on_show, app_manifest.on_hide);
    } else if (event->type == loader::LoaderEventTypeApplicationHiding) {
        hide_app();
    }
}

Gui* gui_alloc() {
    auto* instance = static_cast<Gui*>(malloc(sizeof(Gui)));
    memset(instance, 0, sizeof(Gui));
    tt_check(instance != NULL);
    instance->thread = thread_alloc_ex(
        "gui",
        4096, // Last known minimum was 2800 for launching desktop
        &gui_main,
        nullptr
    );
    instance->mutex = tt_mutex_alloc(MutexTypeRecursive);
    instance->keyboard = nullptr;
    instance->loader_pubsub_subscription = tt_pubsub_subscribe(loader::get_pubsub(), &loader_callback, instance);
    tt_check(lvgl::lock(1000 / portTICK_PERIOD_MS));
    instance->keyboard_group = lv_group_create();
    instance->lvgl_parent = lv_scr_act();
    lvgl::unlock();

    return instance;
}

void gui_free(Gui* instance) {
    tt_assert(instance != nullptr);
    thread_free(instance->thread);
    tt_mutex_free(instance->mutex);

    tt_check(lvgl::lock(1000 / portTICK_PERIOD_MS));
    lv_group_del(instance->keyboard_group);
    lvgl::unlock();

    free(instance);
}

void lock() {
    tt_assert(gui);
    tt_assert(gui->mutex);
    tt_check(tt_mutex_acquire(gui->mutex, configTICK_RATE_HZ) == TtStatusOk);
}

void unlock() {
    tt_assert(gui);
    tt_assert(gui->mutex);
    tt_check(tt_mutex_release(gui->mutex) == TtStatusOk);
}

void request_draw() {
    tt_assert(gui);
    ThreadId thread_id = thread_get_id(gui->thread);
    thread_flags_set(thread_id, GUI_THREAD_FLAG_DRAW);
}

void show_app(App app, ViewPortShowCallback on_show, ViewPortHideCallback on_hide) {
    lock();
    tt_check(gui->app_view_port == NULL);
    gui->app_view_port = view_port_alloc(app, on_show, on_hide);
    unlock();
    request_draw();
}

void hide_app() {
    lock();
    ViewPort* view_port = gui->app_view_port;
    tt_check(view_port != NULL);

    // We must lock the LVGL port, because the viewport hide callbacks
    // might call LVGL APIs (e.g. to remove the keyboard from the screen root)
    tt_check(lvgl::lock(configTICK_RATE_HZ));
    view_port_hide(view_port);
    lvgl::unlock();

    view_port_free(view_port);
    gui->app_view_port = nullptr;
    unlock();
}

static int32_t gui_main(TT_UNUSED void* p) {
    tt_check(gui);
    Gui* local_gui = gui;

    while (true) {
        uint32_t flags = thread_flags_wait(
            GUI_THREAD_FLAG_ALL,
            TtFlagWaitAny,
            TtWaitForever
        );
        // Process and dispatch draw call
        if (flags & GUI_THREAD_FLAG_DRAW) {
            thread_flags_clear(GUI_THREAD_FLAG_DRAW);
            redraw(local_gui);
        }

        if (flags & GUI_THREAD_FLAG_EXIT) {
            thread_flags_clear(GUI_THREAD_FLAG_EXIT);
            break;
        }
    }

    return 0;
}

// region AppManifest

static void start(TT_UNUSED Service& service) {
    gui = gui_alloc();

    thread_set_priority(gui->thread, THREAD_PRIORITY_SERVICE);
    thread_start(gui->thread);
}

static void stop(TT_UNUSED Service& service) {
    lock();

    ThreadId thread_id = thread_get_id(gui->thread);
    thread_flags_set(thread_id, GUI_THREAD_FLAG_EXIT);
    thread_join(gui->thread);
    thread_free(gui->thread);

    unlock();

    gui_free(gui);
}

extern const ServiceManifest manifest = {
    .id = "gui",
    .on_start = &start,
    .on_stop = &stop
};

// endregion

} // namespace
