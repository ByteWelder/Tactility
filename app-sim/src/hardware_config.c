#include "hardware_config.h"
#include "lvgl_task.h"
#include "src/lv_init.h"
#include <stdbool.h>

#define TAG "hardware"

static bool lvgl_init() {
    lv_init();
    lvgl_task_start();
    return true;
}

TT_UNUSED static void lvgl_deinit() {
    lvgl_task_interrupt();
    while (lvgl_task_is_running()) {
        tt_delay_ms(10);
    }

#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    lv_deinit();
#endif
}

HardwareConfig sim_hardware = {
    .bootstrap = NULL,
    .init_lvgl = &lvgl_init,
};
