#include <lvgl/devices/trackball.h>
#include <lvgl/devices/device_context.h>
#include <lvgl/lvgl.h>
#include <tactility/device.h>
#include <tactility/drivers/trackball.h>

#include <Tactility/Assets.h>
#include <Tactility/settings/TrackballSettings.h>

namespace tt::lvgl {

static void initTrackball(lv_indev_t* indev, LvglTrackballSettings& settings) {
    lv_indev_type_t type = lv_indev_get_type(indev);

    void* driver_data = lv_indev_get_driver_data(indev);
    if (!driver_data) {
        return;
    }

    LvglDeviceContext* context = static_cast<LvglDeviceContext*>(driver_data);
    if (!context->device) {
        return;
    }

    const DeviceType* device_type = device_get_type(context->device);
    if (device_type != &TRACKBALL_TYPE) {
        return;
    }

    lvgl_trackball_set_settings(indev, &settings);
    if (settings.mode == LVGL_TRACKBALL_MODE_POINTER) {
        lvgl_trackball_set_cursor_image(indev, TT_ASSETS_UI_CURSOR);
    }
}

void initTrackball() {
    auto trackball_settings = settings::trackball::loadOrGetDefault();
    lv_indev_t* indev = lv_indev_get_next(nullptr);
    while (indev != nullptr) {
        initTrackball(indev, trackball_settings);
        indev = lv_indev_get_next(indev);
    }
}

}