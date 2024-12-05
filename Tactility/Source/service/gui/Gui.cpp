#include "Tactility.h"
#include "service/gui/Gui_i.h"
#include "service/loader/Loader_i.h"
#include "lvgl/LvglKeypad.h"
#include "lvgl/LvglSync.h"
#include "RtosCompat.h"

namespace tt::service::gui {

#define TAG "gui"

// Forward declarations
void redraw(Gui*);
static int32_t gui_main(void*);

Gui* gui = nullptr;

void loader_callback(const void* message, TT_UNUSED void* context) {
    auto* event = static_cast<const loader::LoaderEvent*>(message);
    if (event->type == loader::LoaderEventTypeApplicationShowing) {
        app::AppContext& app = event->app_showing.app;
        const app::AppManifest& app_manifest = app.getManifest();
        showApp(app, app_manifest.onShow, app_manifest.onHide);
    } else if (event->type == loader::LoaderEventTypeApplicationHiding) {
        hideApp();
    }
}

Gui* gui_alloc() {
    auto* instance = static_cast<Gui*>(malloc(sizeof(Gui)));
    memset(instance, 0, sizeof(Gui));
    tt_check(instance != nullptr);
    instance->thread = new Thread(
        "gui",
        4096, // Last known minimum was 2800 for launching desktop
        &gui_main,
        nullptr
    );
    instance->mutex = tt_mutex_alloc(MutexTypeRecursive);
    instance->keyboard = nullptr;
    instance->loader_pubsub_subscription = tt_pubsub_subscribe(loader::getPubsub(), &loader_callback, instance);
    tt_check(lvgl::lock(1000 / portTICK_PERIOD_MS));
    instance->keyboard_group = lv_group_create();
    instance->lvgl_parent = lv_scr_act();
    lvgl::unlock();

    return instance;
}

void gui_free(Gui* instance) {
    tt_assert(instance != nullptr);
    delete instance->thread;
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

void requestDraw() {
    tt_assert(gui);
    ThreadId thread_id = gui->thread->getId();
    thread_flags_set(thread_id, GUI_THREAD_FLAG_DRAW);
}

void showApp(app::AppContext& app, ViewPortShowCallback on_show, ViewPortHideCallback on_hide) {
    lock();
    tt_check(gui->app_view_port == nullptr);
    gui->app_view_port = view_port_alloc(app, on_show, on_hide);
    unlock();
    requestDraw();
}

void hideApp() {
    lock();
    ViewPort* view_port = gui->app_view_port;
    tt_check(view_port != nullptr);

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

static void start(TT_UNUSED ServiceContext& service) {
    gui = gui_alloc();

    gui->thread->setPriority(THREAD_PRIORITY_SERVICE);
    gui->thread->start();
}

static void stop(TT_UNUSED ServiceContext& service) {
    lock();

    ThreadId thread_id = gui->thread->getId();
    thread_flags_set(thread_id, GUI_THREAD_FLAG_EXIT);
    gui->thread->join();
    delete gui->thread;

    unlock();

    gui_free(gui);
}

extern const ServiceManifest manifest = {
    .id = "Gui",
    .onStart = &start,
    .onStop = &stop
};

// endregion

} // namespace
