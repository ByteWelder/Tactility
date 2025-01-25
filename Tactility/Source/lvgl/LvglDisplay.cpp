#include "lvgl/LvglDisplay.h"
#include "Check.h"

namespace tt::lvgl {

hal::Display* getDisplay() {
    auto* lvgl_display = lv_display_get_default();
    assert(lvgl_display != nullptr);
    auto* hal_display = (tt::hal::Display*)lv_display_get_user_data(lvgl_display);
    assert(hal_display != nullptr);
    return hal_display;
}

}
