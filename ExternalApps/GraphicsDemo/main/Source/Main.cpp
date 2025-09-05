#include "Application.h"
#include "drivers/DisplayDriver.h"
#include "drivers/TouchDriver.h"

#include <esp_log.h>

#include <tt_app.h>
#include <tt_app_alertdialog.h>
#include <tt_lvgl.h>

constexpr auto TAG = "Main";

/** Find a DisplayDevice that supports the DisplayDriver interface */
static bool findUsableDisplay(DeviceId& deviceId) {
    uint16_t display_count = 0;
    if (!tt_hal_device_find(DEVICE_TYPE_DISPLAY, &deviceId, &display_count, 1)) {
        ESP_LOGE(TAG, "No display device found");
        return false;
    }

    if (!tt_hal_display_driver_supported(deviceId)) {
        ESP_LOGE(TAG, "Display doesn't support driver mode");
        return false;
    }

    return true;
}

/** Find a TouchDevice that supports the TouchDriver interface */
static bool findUsableTouch(DeviceId& deviceId) {
    uint16_t touch_count = 0;
    if (!tt_hal_device_find(DEVICE_TYPE_TOUCH, &deviceId, &touch_count, 1)) {
        ESP_LOGE(TAG, "No touch device found");
        return false;
    }

    if (!tt_hal_touch_driver_supported(deviceId)) {
        ESP_LOGE(TAG, "Touch doesn't support driver mode");
        return false;
    }

    return true;
}

static void onCreate(AppHandle appHandle, void* data) {
    DeviceId display_id;
    if (!findUsableDisplay(display_id)) {
        tt_app_stop();
        tt_app_alertdialog_start("Error", "The display doesn't support the required features.", nullptr, 0);
        return;
    }

    DeviceId touch_id;
    if (!findUsableTouch(touch_id)) {
        tt_app_stop();
        tt_app_alertdialog_start("Error", "The touch driver doesn't support the required features.", nullptr, 0);
        return;
    }

    // Stop LVGL first (because it's currently using the drivers we want to use)
    tt_lvgl_stop();

    ESP_LOGI(TAG, "Creating display driver");
    auto display = new DisplayDriver(display_id);

    ESP_LOGI(TAG, "Creating touch driver");
    auto touch = new TouchDriver(touch_id);

    // Run the main logic
    ESP_LOGI(TAG, "Running application");
    runApplication(display, touch);

    ESP_LOGI(TAG, "Cleanup display driver");
    delete display;

    ESP_LOGI(TAG, "Cleanup touch driver");
    delete touch;

    ESP_LOGI(TAG, "Stopping application");
    tt_app_stop();
}

static void onDestroy(AppHandle appHandle, void* data) {
    // Restart LVGL to resume rendering of regular apps
    if (!tt_lvgl_is_started()) {
        ESP_LOGI(TAG, "Restarting LVGL");
        tt_lvgl_start();
    }
}

ExternalAppManifest manifest = {
    .name = "Hello World",
    .onCreate = onCreate,
    .onDestroy = onDestroy
};

extern "C" {

int main(int argc, char* argv[]) {
    tt_app_register(&manifest);
    return 0;
}

}
