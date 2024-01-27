#include "lvgl.h"
#include "tactility_core.h"
#include <sdl/sdl.h>

#define TAG "lvgl_hal"

#define BUFFER_SIZE (SDL_HOR_RES * SDL_VER_RES * 3)

lv_disp_t* lvgl_hal_init() {
    // Use the 'monitor' driver to simulate a display on PC
    // Note: this is part of lv_drivers and not SDL!
    sdl_init();

    // Create display buffer
    static lv_disp_draw_buf_t disp_buf1;
    static lv_color_t buf1_1[BUFFER_SIZE];
    lv_disp_draw_buf_init(&disp_buf1, buf1_1, NULL, BUFFER_SIZE);

    // Create display
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv); /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf1;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = SDL_HOR_RES;
    disp_drv.ver_res = SDL_VER_RES;

    lv_disp_t* display = lv_disp_drv_register(&disp_drv);

    lv_theme_t* theme = lv_theme_default_init(
        display,
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        LV_THEME_DEFAULT_DARK,
        LV_FONT_DEFAULT
    );

    lv_disp_set_theme(display, theme);

    lv_group_t* group = lv_group_create();
    lv_group_set_default(group);

    /* Add the mouse as input device
     * Use the 'mouse' driver which reads the PC's mouse*/
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;

    /*This function will be called periodically (by the library) to get the mouse position and state*/
    indev_drv_1.read_cb = sdl_mouse_read;
    lv_indev_t* mouse_indev = lv_indev_drv_register(&indev_drv_1);

    static lv_indev_drv_t indev_drv_2;
    lv_indev_drv_init(&indev_drv_2); /*Basic initialization*/
    indev_drv_2.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_2.read_cb = sdl_keyboard_read;
    lv_indev_t* kb_indev = lv_indev_drv_register(&indev_drv_2);
    lv_indev_set_group(kb_indev, group);

    static lv_indev_drv_t indev_drv_3;
    lv_indev_drv_init(&indev_drv_3); /*Basic initialization*/
    indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
    indev_drv_3.read_cb = sdl_mousewheel_read;
    lv_indev_t* enc_indev = lv_indev_drv_register(&indev_drv_3);
    lv_indev_set_group(enc_indev, group);

    return display;
}

