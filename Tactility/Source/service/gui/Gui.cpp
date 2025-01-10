#include "Tactility.h"
#include "service/gui/Gui_i.h"
#include "service/loader/Loader_i.h"
#include "lvgl/LvglSync.h"
#include "RtosCompat.h"
#include "lvgl/Style.h"
#include "lvgl/Statusbar.h"

namespace tt::service::gui {

#define TAG "gui"

// Forward declarations
void redraw(Gui*);
static int32_t guiMain(TT_UNUSED void* p);

Gui* gui = nullptr;

void onLoaderMessage(const void* message, TT_UNUSED void* context) {
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
    auto* instance = new Gui();
    tt_check(instance != nullptr);
    instance->thread = new Thread(
        "gui",
        4096, // Last known minimum was 2800 for launching desktop
        &guiMain,
        nullptr
    );
    instance->loader_pubsub_subscription = tt_pubsub_subscribe(loader::getPubsub(), &onLoaderMessage, instance);
    tt_check(lvgl::lock(1000 / portTICK_PERIOD_MS));
    instance->keyboardGroup = lv_group_create();
    auto* screen_root = lv_scr_act();

    lvgl::obj_set_style_bg_blacken(screen_root);

    lv_obj_t* vertical_container = lv_obj_create(screen_root);
    lv_obj_set_size(vertical_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(vertical_container, LV_FLEX_FLOW_COLUMN);
    lvgl::obj_set_style_no_padding(vertical_container);
    lvgl::obj_set_style_bg_blacken(vertical_container);

    instance->statusbarWidget = lvgl::statusbar_create(vertical_container);

    auto* app_container = lv_obj_create(vertical_container);
    lvgl::obj_set_style_no_padding(app_container);
    lv_obj_set_style_border_width(app_container, 0, 0);
    lvgl::obj_set_style_bg_blacken(app_container);
    lv_obj_set_width(app_container, LV_PCT(100));
    lv_obj_set_flex_grow(app_container, 1);
    lv_obj_set_flex_flow(app_container, LV_FLEX_FLOW_COLUMN);

    instance->appRootWidget = app_container;

    lvgl::unlock();

    return instance;
}

void gui_free(Gui* instance) {
    tt_assert(instance != nullptr);
    delete instance->thread;

    lv_group_delete(instance->keyboardGroup);
    tt_check(lvgl::lock(1000 / portTICK_PERIOD_MS));
    lv_group_del(instance->keyboardGroup);
    lvgl::unlock();

    delete instance;
}

void lock() {
    tt_assert(gui);
    tt_check(gui->mutex.acquire(configTICK_RATE_HZ) == TtStatusOk);
}

void unlock() {
    tt_assert(gui);
    tt_check(gui->mutex.release() == TtStatusOk);
}

void requestDraw() {
    tt_assert(gui);
    ThreadId thread_id = gui->thread->getId();
    thread_flags_set(thread_id, GUI_THREAD_FLAG_DRAW);
}

void showApp(app::AppContext& app, ViewPortShowCallback on_show, ViewPortHideCallback on_hide) {
    lock();
    tt_check(gui->appViewPort == nullptr);
    gui->appViewPort = view_port_alloc(app, on_show, on_hide);
    unlock();
    requestDraw();
}

void hideApp() {
    lock();
    ViewPort* view_port = gui->appViewPort;
    tt_check(view_port != nullptr);

    // We must lock the LVGL port, because the viewport hide callbacks
    // might call LVGL APIs (e.g. to remove the keyboard from the screen root)
    tt_check(lvgl::lock(configTICK_RATE_HZ));
    view_port_hide(view_port);
    lvgl::unlock();

    view_port_free(view_port);
    gui->appViewPort = nullptr;
    unlock();
}

static int32_t guiMain(TT_UNUSED void* p) {
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
