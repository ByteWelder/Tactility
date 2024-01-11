#include "wifi_view.h"

#include "ui/style.h"
#include "wifi_connect_view.h"
#include "wifi_main_view.h"

#define TAG "wifi_view"

// region Main

void wifi_view_set_active(WifiView* view, WifiActiveScreen active) {
    if (active == WIFI_SCREEN_MAIN) {
        lv_obj_clear_flag(view->main_view.root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->connect_view.root, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(view->connect_view.root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->main_view.root, LV_OBJ_FLAG_HIDDEN);
    }
}

void wifi_view_create(WifiView* view, WifiBindings* bindings, lv_obj_t* parent) {
    view->root = parent;

    tt_lv_obj_set_style_no_padding(parent);

    // Main View

    lv_obj_t* main_view_root = lv_obj_create(parent);
    lv_obj_set_size(main_view_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_border_width(main_view_root, 0, 0);
    tt_lv_obj_set_style_no_padding(main_view_root);
    view->main_view.root = main_view_root;

    wifi_main_view_create(view, bindings, main_view_root);

    // Connect View

    lv_obj_t* connect_view_root = lv_obj_create(parent);
    lv_obj_set_size(connect_view_root, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(connect_view_root, LV_OBJ_FLAG_HIDDEN);
    view->connect_view.root = connect_view_root;

    wifi_connect_view_create(view, bindings, connect_view_root);
}

void wifi_view_update(WifiView* view, WifiBindings* bindings, WifiState* state) {
    wifi_view_set_active(view, state->active_screen);
    if (state->active_screen == WIFI_SCREEN_MAIN) {
        wifi_main_view_update(view, state, bindings);
    } else if (state->active_screen == WIFI_SCREEN_CONNECT) {
        wifi_connect_view_update(view, state);
    }
}

// endregion Main