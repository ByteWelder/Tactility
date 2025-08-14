#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <lvgl.h>
#include <Tactility/hal/Device.h>
#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/service/ServiceRegistry.h>

using namespace tt::app;

class HelloWorldApp : public App {

    std::shared_ptr<tt::hal::display::DisplayDevice> displayDevice;
    // void onShow(AppContext& context, lv_obj_t* parent) override {
    //     lv_obj_t* toolbar = tt::lvgl::toolbar_create(parent, context);
    //     lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);
    //
    //     lv_obj_t* label = lv_label_create(parent);
    //     lv_label_set_text(label, "Hello, world!");
    //     lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    // }

    void onCreate(AppContext& appContext) override {
        tt::service::stopService("Statusbar");
        tt::service::stopService("Gui");

        tt::service::startService("Gui");
        tt::service::startService("Statusbar");

    //     using namespace tt::hal;
    //     displayDevice = findFirstDevice<display::DisplayDevice>(Device::Type::Display);
    //     if (displayDevice == nullptr) {
    //         TT_LOG_E("HelloWorld", "Display device not found");
    //         stop();
    //     } else {
    //         if (displayDevice->supportsLvgl() && displayDevice->getLvglDisplay() != nullptr) {
    //             if (!displayDevice->stopLvgl()) {
    //                 TT_LOG_E("HelloWorld", "Failed to detach display from LVGL");
    //             }
    //         }
    //     }
    }

    void onDestroy(AppContext& appContext) override {
        if (displayDevice != nullptr) {
            if (displayDevice->supportsLvgl() && displayDevice->getLvglDisplay() == nullptr) {
                displayDevice->startLvgl();
            }
        }
    }
};

extern const AppManifest hello_world_app = {
    .id = "HelloWorld",
    .name = "Hello World",
    .createApp = create<HelloWorldApp>
};
