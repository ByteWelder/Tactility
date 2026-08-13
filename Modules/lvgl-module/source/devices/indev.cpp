#include <lvgl/devices/indev_private.h>

extern "C" {

bool lvgl_has_indev_of_type(lv_indev_type_t type) {
    for (lv_indev_t* indev = lv_indev_get_next(nullptr); indev != nullptr; indev = lv_indev_get_next(indev)) {
        lv_indev_type_t to_check = lv_indev_get_type(indev);
        if (to_check == type) {
            return true;
        }
    }
    return false;
}

} // extern "C"
