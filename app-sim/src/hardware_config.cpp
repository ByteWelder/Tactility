#include "hal/Configuration.h"
#include "lvgl_task.h"
#include "src/lv_init.h"

#define TAG "hardware"

extern const tt::hal::Power power;

static bool lvgl_init() {
    lv_init();
    lvgl_task_start();
    return true;
}

TT_UNUSED static void lvgl_deinit() {
    lvgl_task_interrupt();
    while (lvgl_task_is_running()) {
        tt::delay_ms(10);
    }

#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    lv_deinit();
#endif
}

extern const tt::hal::Configuration sim_hardware = {
    .bootstrap = nullptr,
    .init_graphics = &lvgl_init,
    .display = {
        .set_backlight_duty = nullptr,
    },
    .sdcard = nullptr,
    .power = &power
};
