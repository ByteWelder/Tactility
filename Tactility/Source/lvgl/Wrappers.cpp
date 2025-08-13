#include <lvgl.h>
#include <Tactility/service/gui/Gui.h>

extern "C" {

extern lv_obj_t * __real_lv_textarea_create(lv_obj_t * parent);

lv_obj_t * __wrap_lv_textarea_create(lv_obj_t * parent) {
    auto textarea = __real_lv_textarea_create(parent);
    tt::service::gui::keyboardAddTextArea(textarea);
    return textarea;
}

}