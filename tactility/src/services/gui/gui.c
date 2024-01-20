#include "gui_i.h"

#include "check.h"
#include "core_extra_defines.h"
#include "ui/lvgl_sync.h"
#include "kernel.h"
#include "log.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#else
#include "FreeRTOS.h"
#endif

#define TAG "gui"

// Forward declarations
void gui_redraw(Gui*);
static int32_t gui_main(void*);

static Gui* gui = NULL;

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

    tt_check(tt_lvgl_lock(100));
    instance->lvgl_parent = lv_scr_act();
    tt_lvgl_unlock();

    return instance;
}

void gui_free(Gui* instance) {
    tt_assert(instance != NULL);
    tt_thread_free(instance->thread);
    tt_mutex_free(instance->mutex);
    free(instance);
}

void gui_lock() {
    tt_assert(gui);
    tt_assert(gui->mutex);
    tt_check(tt_mutex_acquire(gui->mutex, 1000 / portTICK_PERIOD_MS) == TtStatusOk);
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

void gui_keyboard_show(lv_obj_t* textarea) {
    if (gui->keyboard) {
        gui_lock();

        lv_obj_clear_flag(gui->keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(gui->keyboard, textarea);

        if (gui->toolbar) {
            lv_obj_add_flag(gui->toolbar, LV_OBJ_FLAG_HIDDEN);
        }

        gui_unlock();
    }
}

void gui_keyboard_hide() {
    if (gui->keyboard) {
        gui_lock();

        lv_obj_add_flag(gui->keyboard, LV_OBJ_FLAG_HIDDEN);

        if (gui->toolbar) {
            lv_obj_clear_flag(gui->toolbar, LV_OBJ_FLAG_HIDDEN);
        }

        gui_unlock();
    }
}

void gui_hide_app() {
    gui_lock();
    ViewPort* view_port = gui->app_view_port;
    tt_check(view_port != NULL);

    // We must lock the LVGL port, because the viewport hide callbacks
    // might call LVGL APIs (e.g. to remove the keyboard from the screen root)
    tt_check(tt_lvgl_lock(1000));
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

    tt_thread_set_priority(gui->thread, ThreadPriorityNormal);
    tt_thread_start(gui->thread);
}

static void gui_stop(TT_UNUSED Service service) {
    gui_lock();

    ThreadId thread_id = tt_thread_get_id(gui->thread);
    tt_thread_flags_set(thread_id, GUI_THREAD_FLAG_EXIT);
    tt_thread_join(gui->thread);

    gui_unlock();

    gui_free(gui);
}

const ServiceManifest gui_service = {
    .id = "gui",
    .on_start = &gui_start,
    .on_stop = &gui_stop
};

// endregion
