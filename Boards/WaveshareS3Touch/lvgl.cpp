#include "lvgl_i.h"

#include "display_i.h"
#include "touch_i.h"
#include "ui/LvglSync.h"

bool ws3t_init_lvgl() {
    tt::lvgl::sync_set(&ws3t_display_lock, &ws3t_display_unlock);

    lv_display_t* display = ws3t_display_create();
    if (display == nullptr) {
        return false;
    }

    ws3t_touch_init(display);

    return true;
}
