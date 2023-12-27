#include "gui.h"
#include "core_defines.h"
#include <record.h>
#include <mutex.h>
#include <check.h>
#include <m-dict.h>
#include <m-core.h>

typedef struct screen screen_t;
struct screen {
    screen_id_t id;
    lv_obj_t* parent;
    on_init_lvgl _Nonnull callback;
};

static screen_id_t screen_counter = 0;

DICT_DEF2(screen_dict, screen_id_t, M_BASIC_OPLIST, screen_t, M_POD_OPLIST)

typedef struct Gui Gui;
struct Gui {
    // TODO: use mutex
    FuriMutex* mutex;
    screen_dict_t screens;
};

Gui* gui_alloc() {
    Gui* gui = malloc(sizeof(Gui));
    screen_dict_init(gui->screens);
    gui->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    return gui;
}

void gui_free(Gui* gui) {
    screen_dict_clear(gui->screens);
    furi_mutex_free(gui->mutex);
    free(gui);
}

void gui_lock(Gui* gui) {
    furi_assert(gui);
    furi_check(furi_mutex_acquire(gui->mutex, FuriWaitForever) == FuriStatusOk);
}

void gui_unlock(Gui* gui) {
    furi_assert(gui);
    furi_check(furi_mutex_release(gui->mutex) == FuriStatusOk);
}

screen_id_t gui_screen_create(Gui* gui, on_init_lvgl callback) {
    screen_id_t id = screen_counter++;
    screen_t screen = {
        .id = id,
        .parent = NULL,
        .callback = callback
    };

    screen_dict_set_at(gui->screens, id, screen);

    // TODO: notify desktop of change
    // TODO: have desktop update views
    lv_obj_t* parent = lv_scr_act();
    gui_screen_set_parent(gui, id, parent);

    // TODO: call from desktop
    screen.callback(gui_screen_get_parent(gui, id), id);

    return id;
}

lv_obj_t* gui_screen_get_parent(Gui* gui, screen_id_t id) {
    screen_t* screen = screen_dict_get(gui->screens, id);
    furi_check(screen != NULL);
    return screen->parent;
}

void gui_screen_set_parent(Gui* gui, screen_id_t id, lv_obj_t* parent) {
    screen_t* screen = screen_dict_get(gui->screens, id);
    furi_check(screen != NULL);
    screen->parent = parent;
}

void gui_screen_free(Gui* gui, screen_id_t id) {
    screen_t* screen = screen_dict_get(gui->screens, id);
    furi_check(screen != NULL);

    // TODO: notify? use callback? (done from desktop service)
    lv_obj_clean(screen->parent);

    screen_dict_erase(gui->screens, id);
}

static int32_t prv_gui_main(void* param) {
    UNUSED(param);

    Gui* gui = gui_alloc();
    furi_record_create(RECORD_GUI, gui);
    printf("gui app init\n");
    return 0;
}

const nb_app_t gui_app = {
    .id = "gui",
    .name = "GUI",
    .type = SERVICE,
    .entry_point = &prv_gui_main,
    .stack_size = 2048,
    .priority = 10
};
