#include "wifi_view.h"

void wifi_view_create(WifiView* view, lv_obj_t* parent) {
    lv_obj_t* spinner = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(spinner, 32, 32);
    lv_obj_center(spinner);
    view->spinner = spinner;
}

void wifi_view_update(WifiView* view, WifiState* state) {
    if (state->scanning) {
        lv_obj_clear_flag(view->spinner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(view->spinner, LV_OBJ_FLAG_HIDDEN);
    }
}

void wifi_view_clear(WifiView* view) {
    view->spinner = NULL;
}
