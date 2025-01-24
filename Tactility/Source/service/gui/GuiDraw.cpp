#include "Check.h"
#include "Log.h"
#include "service/gui/Gui_i.h"
#include "lvgl/LvglSync.h"
#include "lvgl/Statusbar.h"
#include "lvgl/Style.h"

namespace tt::service::gui {

#define TAG "gui"

static lv_obj_t* createAppViews(Gui* gui, lv_obj_t* parent) {
    lv_obj_send_event(gui->statusbarWidget, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_t* child_container = lv_obj_create(parent);
    lv_obj_set_style_pad_all(child_container, 0, 0);
    lv_obj_set_width(child_container, LV_PCT(100));
    lv_obj_set_flex_grow(child_container, 1);

    if (keyboardIsEnabled()) {
        gui->keyboard = lv_keyboard_create(parent);
        lv_obj_add_flag(gui->keyboard, LV_OBJ_FLAG_HIDDEN);
    } else {
        gui->keyboard = nullptr;
    }

    return child_container;
}

void redraw(Gui* gui) {
    assert(gui);

    // Lock GUI and LVGL
    lock();

    if (lvgl::lock(1000)) {
        lv_obj_clean(gui->appRootWidget);

        if (gui->appToRender != nullptr) {

            app::Flags flags = std::static_pointer_cast<app::AppInstance>(gui->appToRender)->getFlags();
            if (flags.showStatusbar) {
                lv_obj_remove_flag(gui->statusbarWidget, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(gui->statusbarWidget, LV_OBJ_FLAG_HIDDEN);
            }

            lv_obj_t* container = createAppViews(gui, gui->appRootWidget);
            gui->appToRender->getApp()->onShow(*gui->appToRender, container);
        } else {
            TT_LOG_W(TAG, "nothing to draw");
        }

        // Unlock GUI and LVGL
        lvgl::unlock();
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "LVGL");
    }

    unlock();
}

} // namespace
