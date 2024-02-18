#include "lvgl.h"
#include "ui/lvgl_keypad.h"

lv_display_t* lvgl_hal_init() {
    static lv_display_t* display = NULL;
    static lv_indev_t* mouse = NULL;
    static lv_indev_t* keyboard = NULL;

    display = lv_sdl_window_create(320, 240);
    mouse = lv_sdl_mouse_create();
    keyboard = lv_sdl_keyboard_create();
    tt_lvgl_keypad_set_indev(keyboard);

    return display;
}
