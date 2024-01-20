#include "lvgl_hal.h"

#include "lvgl.h"
#include "tactility_core.h"
#include "thread.h"
#include <sdl/sdl.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"

#define TAG "lvgl_hal"

extern const lv_img_dsc_t mouse_cursor_icon;
static void lvgl_task_deinit();

static QueueHandle_t lvgl_mutex = NULL;
static uint32_t task_max_sleep_ms = 100;
static bool task_running = true;

#define BUFFER_SIZE (SDL_HOR_RES * SDL_VER_RES * 3)

static lv_disp_t* hal_init() {
    /* Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
    sdl_init();

    /*Create a display buffer*/
    static lv_disp_draw_buf_t disp_buf1;
    static lv_color_t buf1_1[BUFFER_SIZE];
    lv_disp_draw_buf_init(&disp_buf1, buf1_1, NULL, BUFFER_SIZE);

    /*Create a display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv); /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf1;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = SDL_HOR_RES;
    disp_drv.ver_res = SDL_VER_RES;

    lv_disp_t* disp = lv_disp_drv_register(&disp_drv);

    lv_theme_t* th = lv_theme_default_init(
        disp,
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        LV_THEME_DEFAULT_DARK,
        LV_FONT_DEFAULT
    );

    lv_disp_set_theme(disp, th);

    lv_group_t* g = lv_group_create();
    lv_group_set_default(g);

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
    lv_indev_set_group(kb_indev, g);

    static lv_indev_drv_t indev_drv_3;
    lv_indev_drv_init(&indev_drv_3); /*Basic initialization*/
    indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
    indev_drv_3.read_cb = sdl_mousewheel_read;
    lv_indev_t* enc_indev = lv_indev_drv_register(&indev_drv_3);
    lv_indev_set_group(enc_indev, g);

    /*Set a cursor for the mouse*/
    LV_IMG_DECLARE(mouse_cursor_icon);                  /*Declare the image file.*/
    lv_obj_t* cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
    lv_img_set_src(cursor_obj, &mouse_cursor_icon);     /*Set the image source*/
    lv_indev_set_cursor(mouse_indev, cursor_obj);       /*Connect the image  object to the driver*/
    return disp;
}

static void lvgl_init() {
    TT_LOG_I(TAG, "init: started");
    lv_init();
    hal_init();
    tt_check(lvgl_mutex == NULL);
    TT_LOG_D(TAG, "init: creating mutex");
    lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    //    TT_LOG_D(TAG, "init: starting task");
    //    xTaskCreate(lvgl_task, "lvgl_task", 9192, NULL, 3, NULL);
    TT_LOG_I(TAG, "init: complete");
}

void lvgl_task(TT_UNUSED void* arg) {
    lvgl_init();

    uint32_t task_delay_ms = task_max_sleep_ms;

    task_running = true;
    while (task_running) {
        if (lvgl_lock(0)) {
            task_delay_ms = lv_timer_handler();
            lvgl_unlock();
        }
        if ((task_delay_ms > task_max_sleep_ms) || (1 == task_delay_ms)) {
            task_delay_ms = task_max_sleep_ms;
        } else if (task_delay_ms < 1) {
            task_delay_ms = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }

    lvgl_task_deinit();

    /* Close task */
    vTaskDelete(NULL);
}

static void lvgl_task_deinit() {
    if (lvgl_mutex) {
        vSemaphoreDelete(lvgl_mutex);
    }
#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    lv_deinit();
#endif
}

void lvgl_interrupt() {
    tt_check(lvgl_lock(TtWaitForever));
    task_running = false;
    lvgl_unlock();
}

bool lvgl_lock(int timeout_ticks) {
    return xSemaphoreTakeRecursive(lvgl_mutex, timeout_ticks) == pdTRUE;
}

void lvgl_unlock() {
    xSemaphoreGiveRecursive(lvgl_mutex);
}
