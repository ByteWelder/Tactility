#include "Tactility/app/App.h"
#include "Tactility/app/AppManifest.h"
#include "Tactility/lvgl/Toolbar.h"

#include <Tactility/CoreDefines.h>
#include <Tactility/hal/usb/Usb.h>

#include <lvgl.h>

#define TAG "usb_settings"

namespace tt::app::usbsettings {

static void onRebootMassStorageSdmmc(TT_UNUSED lv_event_t* event) {
    hal::usb::rebootIntoMassStorageSdmmc();
}

// Flash reboot handler
static void onRebootMassStorageFlash(TT_UNUSED lv_event_t* event) {
    hal::usb::rebootIntoMassStorageFlash();
}

class UsbSettingsApp : public App {

    void onShow(AppContext& app, lv_obj_t* parent) override {
        auto* toolbar = lvgl::toolbar_create(parent, app);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        if (hal::usb::canRebootIntoMassStorageSdmmc()) {
            // Existing SDMMC button
            auto* button_sd = lv_button_create(parent);
            auto* label_sd = lv_label_create(button_sd);
            lv_label_set_text(label_sd, "Reboot as USB storage (SD)");
            lv_obj_align(button_sd, LV_ALIGN_CENTER, 0, -20);  // Above flash button
            lv_obj_add_event_cb(button_sd, onRebootMassStorageSdmmc, LV_EVENT_SHORT_CLICKED, nullptr);
        }

        if (hal::usb::canRebootIntoMassStorageFlash()) {
            // Flash button (show even without SD)
            auto* button_flash = lv_button_create(parent);
            auto* label_flash = lv_label_create(button_flash);
            lv_label_set_text(label_flash, "Reboot as USB storage (Flash)");
            lv_obj_align(button_flash, LV_ALIGN_CENTER, 0, 20);  // Below SD button or center if no SD
            lv_obj_add_event_cb(button_flash, onRebootMassStorageFlash, LV_EVENT_SHORT_CLICKED, nullptr);
        }

        if (!hal::usb::canRebootIntoMassStorageSdmmc() && !hal::usb::canRebootIntoMassStorageFlash()) {
            // Fallback error
            bool supported = hal::usb::isSupported();
            const char* first = supported ? "USB storage not available" : "USB driver not supported";
            auto* label_a = lv_label_create(parent);
            lv_label_set_text(label_a, first);
            lv_obj_align(label_a, LV_ALIGN_CENTER, 0, 0);
        }
    }
};

extern const AppManifest manifest = {
    .appId = "UsbSettings",
    .appName = "USB",
    .appIcon = LV_SYMBOL_USB,
    .appCategory = Category::Settings,
    .createApp = create<UsbSettingsApp>
};

} // namespace
