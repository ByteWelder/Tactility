#include "gui_i.h"
#include "core_defines.h"
#include <record.h>
#include <check.h>

static ScreenId screen_counter = 0;

NbGuiHandle gui_alloc() {
    struct NbGui* gui = malloc(sizeof(struct NbGui));
    screen_dict_init(gui->screens);
    gui->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    return gui;
}

void gui_free(NbGuiHandle gui) {
    screen_dict_clear(gui->screens);
    furi_mutex_free(gui->mutex);
    free(gui);
}

void gui_lock(NbGuiHandle gui) {
    furi_assert(gui);
    furi_check(furi_mutex_acquire(gui->mutex, FuriWaitForever) == FuriStatusOk);
}

void gui_unlock(NbGuiHandle gui) {
    furi_assert(gui);
    furi_check(furi_mutex_release(gui->mutex) == FuriStatusOk);
}

ScreenId gui_screen_create(NbGuiHandle gui, InitScreen callback) {
    ScreenId id = screen_counter++;
    NbScreen screen = {
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

lv_obj_t* gui_screen_get_parent(NbGuiHandle gui, ScreenId id) {
    NbScreen* screen = screen_dict_get(gui->screens, id);
    furi_check(screen != NULL);
    return screen->parent;
}

void gui_screen_set_parent(NbGuiHandle gui, ScreenId id, lv_obj_t* parent) {
    NbScreen* screen = screen_dict_get(gui->screens, id);
    furi_check(screen != NULL);
    screen->parent = parent;
}

void gui_screen_free(NbGuiHandle gui, ScreenId id) {
    NbScreen* screen = screen_dict_get(gui->screens, id);
    furi_check(screen != NULL);

    // TODO: notify? use callback? (done from desktop service)
    lv_obj_clean(screen->parent);

    screen_dict_erase(gui->screens, id);
}

static int32_t prv_gui_main(void* param) {
    UNUSED(param);

    struct NbGui* gui = gui_alloc();
    furi_record_create(RECORD_GUI, gui);
    printf("gui app init\n");
    return 0;
}

const NbApp gui_app = {
    .id = "gui",
    .name = "GUI",
    .type = SERVICE,
    .entry_point = &prv_gui_main,
    .stack_size = 2048,
    .priority = 10
};
