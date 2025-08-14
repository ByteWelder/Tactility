#include <lvgl.h>
#include <Tactility/service/gui/GuiService.h>

extern "C" {

extern lv_obj_t * __real_lv_textarea_create(lv_obj_t * parent);

lv_obj_t * __wrap_lv_textarea_create(lv_obj_t * parent) {
    auto textarea = __real_lv_textarea_create(parent);

    auto gui_service = tt::service::gui::findService();
    if (gui_service != nullptr) {
        gui_service->keyboardAddTextArea(textarea);
    }

    return textarea;
}

}