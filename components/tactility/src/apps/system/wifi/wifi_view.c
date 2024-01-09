#include "wifi_view.h"
#include "services/gui/widgets/widgets.h"

#define TAG "wifi_view"

void wifi_view_show_connect(WifiView* view, const char* ssid) {
//    view->active_view = WIFI_VIEW_CONNECT;
//    wifi_view_update(view, state);
}

// region Main

void wifi_view_create(WifiView* view, lv_obj_t* parent) {
    view->root = parent;
    view->active_view = WIFI_VIEW_MAIN;

    lv_obj_set_style_no_padding(parent);

    // Main View

    lv_obj_t* main_view_root = lv_obj_create(parent);
    lv_obj_set_size(main_view_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_border_width(main_view_root, 0, 0);
    lv_obj_set_style_no_padding(main_view_root);
    view->main_view.root = main_view_root;

    wifi_main_view_create(&view->main_view, main_view_root);

    // Connect View

    lv_obj_t* connect_view_root = lv_obj_create(parent);
    lv_obj_set_size(connect_view_root, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(connect_view_root, LV_OBJ_FLAG_HIDDEN);
    view->connect_view.root = connect_view_root;

    wifi_connect_view_create(&view->connect_view, connect_view_root);
}

void wifi_view_update(WifiView* view, WifiState* state) {
    if (view->active_view == WIFI_VIEW_MAIN) {
        wifi_main_view_update(&view->main_view, state);
    } else if (view->active_view == WIFI_VIEW_CONNECT){
        wifi_connect_view_update(&view->connect_view, state);
    }
}

void wifi_view_clear(WifiView* view) {
    wifi_main_view_clear(&view->main_view);
    wifi_connect_view_clear(&view->connect_view);
}

// endregion Main