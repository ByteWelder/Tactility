#include "I2cScanner.h"
#include "I2cScannerThread.h"
#include "I2cHelpers.h"

#include "Assets.h"
#include "Tactility.h"
#include "app/App.h"
#include "lvgl/LvglSync.h"
#include "lvgl/Toolbar.h"

#define START_SCAN_TEXT "Scan"
#define STOP_SCAN_TEXT "Stop scan"

namespace tt::app::i2cscanner {

static void updateViews(Data* data);

static void onSelectBus(lv_event_t* event) {
    auto* dropdown = static_cast<lv_obj_t*>(lv_event_get_target(event));
    uint32_t selected = lv_dropdown_get_selected(dropdown);
    Data* data = (Data*) lv_event_get_user_data(event);
    auto i2c_devices = tt::getConfiguration()->hardware->i2c;
    assert(selected < i2c_devices.size());

    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        data->scannedAddresses.clear();
        data->port = i2c_devices[selected].port;
        data->scanState = ScanStateInitial;
        tt_check(data->mutex.release() == TtStatusOk);

        updateViews(data);
    }

    TT_LOG_I(TAG, "Selected %ld", selected);
}

static void onPressScan(lv_event_t* event) {
    auto* data = (Data*)lv_event_get_user_data(event);
    if (data->scanState == ScanStateScanning) {
        stopScanning(data);
    } else {
        startScanning(data);
    }
    updateViews(data);
}

static void updateViews(Data* data) {
    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        if (data->scanState == ScanStateScanning) {
            lv_label_set_text(data->scanButtonLabel, STOP_SCAN_TEXT);
            lv_obj_remove_flag(data->portDropdown, LV_OBJ_FLAG_CLICKABLE);
        } else {
            lv_label_set_text(data->scanButtonLabel, START_SCAN_TEXT);
            lv_obj_add_flag(data->portDropdown, LV_OBJ_FLAG_CLICKABLE);
        }

        lv_obj_clean(data->scanList);
        if (data->scanState == ScanStateStopped) {
            lv_obj_remove_flag(data->scanList, LV_OBJ_FLAG_HIDDEN);
            if (!data->scannedAddresses.empty()) {
                for (auto address: data->scannedAddresses) {
                    std::string address_text = getAddressText(address);
                    lv_list_add_text(data->scanList, address_text.c_str());
                }
            } else {
                lv_list_add_text(data->scanList, "No devices found");
            }
        } else {
            lv_obj_add_flag(data->scanList, LV_OBJ_FLAG_HIDDEN);
        }

        tt_check(data->mutex.release() == TtStatusOk);
    } else {
        TT_LOG_W(TAG, "updateViews lock");
    }
}

static void updateViewsSafely(Data* data) {
    if (lvgl::lock(100 / portTICK_PERIOD_MS)) {
        updateViews(data);
        lvgl::unlock();
    } else {
        TT_LOG_W(TAG, "updateViewsSafely lock LVGL");
    }
}

void onThreadFinished(Data* data) {
    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        if (data->scanState == ScanStateScanning) {
            data->scanState = ScanStateStopped;
            updateViewsSafely(data);
        }
        tt_check(data->mutex.release() == TtStatusOk);
    } else {
        TT_LOG_W(TAG, "onThreadFinished lock");
    }
}

static void onShow(App& app, lv_obj_t* parent) {
    auto* data = (Data*)app.getData();
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    lvgl::toolbar_create(parent, app);

    lv_obj_t* main_wrapper = lv_obj_create(parent);
    lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(main_wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(main_wrapper, 1);

    lv_obj_t* wrapper = lv_obj_create(main_wrapper);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_height(wrapper, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(wrapper, 0, 0);
    lv_obj_set_style_border_width(wrapper, 0, 0);

    lv_obj_t* scan_button = lv_button_create(wrapper);
    lv_obj_set_width(scan_button, LV_PCT(48));
    lv_obj_align(scan_button, LV_ALIGN_TOP_LEFT, 0, 1); // Shift 1 pixel to align with selection box
    lv_obj_add_event_cb(scan_button, &onPressScan, LV_EVENT_CLICKED, data);
    lv_obj_t* scan_button_label = lv_label_create(scan_button);
    lv_obj_align(scan_button_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(scan_button_label, START_SCAN_TEXT);
    data->scanButtonLabel = scan_button_label;

    lv_obj_t* port_dropdown = lv_dropdown_create(wrapper);
    std::string dropdown_items = getPortNamesForDropdown();
    lv_dropdown_set_options(port_dropdown, dropdown_items.c_str());
    lv_obj_set_width(port_dropdown, LV_PCT(48));
    lv_obj_align(port_dropdown, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(port_dropdown, onSelectBus, LV_EVENT_VALUE_CHANGED, data);
    lv_dropdown_set_selected(port_dropdown, 0);
    data->portDropdown = port_dropdown;

    lv_obj_t* scan_list = lv_list_create(main_wrapper);
    lv_obj_set_style_margin_top(scan_list, 8, 0);
    lv_obj_set_width(scan_list, LV_PCT(100));
    lv_obj_set_height(scan_list, LV_SIZE_CONTENT);
    lv_obj_add_flag(scan_list, LV_OBJ_FLAG_HIDDEN);
    data->scanList = scan_list;
}

static void onHide(App& app) {
    auto* data = (Data*)app.getData();
    if (hasScanThread(data)) {
        stopScanning(data);
    }
}

static void onStart(App& app) {
    Data* data = new Data();
    app.setData(data);
}

static void onStop(App& app) {
    Data* data = (Data*)app.getData();
    delete data;
}

extern const Manifest manifest = {
    .id = "I2cScanner",
    .name = "I2C Scanner",
    .icon = TT_ASSETS_APP_ICON_I2C_SETTINGS,
    .type = TypeSystem,
    .onStart = onStart,
    .onStop = onStop,
    .onShow = onShow,
    .onHide = onHide
};

} // namespace
