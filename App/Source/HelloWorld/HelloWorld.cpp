#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <lvgl.h>

using namespace tt::app;

class HelloWorldApp : public App {

    void onShow(AppContext& context, lv_obj_t* parent) override {
        lv_obj_t* toolbar = tt::lvgl::toolbar_create(parent, context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t* label = lv_label_create(parent);
        lv_label_set_text(label, "Hello, world!");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }
};

extern const AppManifest hello_world_app = {
    .appId = "HelloWorld",
    .appName = "Hello World",
    .createApp = create<HelloWorldApp>
};
