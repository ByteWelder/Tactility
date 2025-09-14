#ifdef ESP_PLATFORM

#include <Tactility/Tactility.h>

#include <lvgl.h>

extern "C" {

extern lv_obj_t* __real_lv_dropdown_create(lv_obj_t* parent);

lv_obj_t* __wrap_lv_dropdown_create(lv_obj_t* parent) {
    auto dropdown = __real_lv_dropdown_create(parent);

    if (tt::hal::getConfiguration()->uiScale == tt::hal::UiScale::Smallest) {
    }

    return dropdown;
}

}

#endif // ESP_PLATFORM