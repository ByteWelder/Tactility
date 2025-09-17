#include "Tactility/app/App.h"
#include "Tactility/app/AppManifest.h"
#include "Tactility/lvgl/Toolbar.h"

#include <Tactility/CoreDefines.h>
#include <Tactility/hal/usb/Usb.h>

#include <lvgl.h>

#define TAG "usb_settings"

namespace tt::app::usbsettings {

static void onRebootMassStorage(TT_UNUSED lv_event_t* event) {
    hal::usb::rebootIntoMassStorageSdmmc();
}

class UsbSettingsApp : public App {

    void onShow(AppContext& app, lv_obj_t* parent) override {
        auto* toolbar = lvgl::toolbar_create(parent, app);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        if (hal::usb::canRebootIntoMassStorageSdmmc()) {
            auto* button = lv_button_create(parent);
            auto* label = lv_label_create(button);
            lv_label_set_text(label, "Reboot as USB storage");
            lv_obj_align(button, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_event_cb(button, onRebootMassStorage, LV_EVENT_SHORT_CLICKED, nullptr);
        } else {
            bool supported = hal::usb::isSupported();
            const char* first = supported ? "USB storage not available:" : "USB driver not supported";
            const char* second = supported ? "SD card not mounted" : "on this hardware";
            auto* label_a = lv_label_create(parent);
            lv_label_set_text(label_a, first);
            lv_obj_align(label_a, LV_ALIGN_CENTER, 0, 0);
            auto* label_b = lv_label_create(parent);
            lv_label_set_text(label_b, second);
            lv_obj_align_to(label_b, label_a, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
        }
    }
};

extern const AppManifest manifest = {
    .id = "UsbSettings",
    .name = "USB",
    .icon = LV_SYMBOL_USB,
    .category = Category::Settings,
    .createApp = create<UsbSettingsApp>
};

} // namespace
