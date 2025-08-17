#include "Application.h"
#include "drivers/DisplayDriver.h"
#include "drivers/TouchDriver.h"

#include <esp_log.h>

#include <tt_app.h>
#include <tt_lvgl.h>

constexpr auto TAG = "Main";

static void onCreate(AppHandle appHandle, void* data) {
    uint16_t display_count = 0;
    DeviceId display_id;
    if (!tt_hal_device_find(DEVICE_TYPE_DISPLAY, &display_id, &display_count, 1)) {
        ESP_LOGI(TAG, "No display device found");
        return;
    }

    uint16_t touch_count = 0;
    DeviceId touch_id;
    if (!tt_hal_device_find(DEVICE_TYPE_TOUCH, &touch_id, &touch_count, 1)) {
        ESP_LOGI(TAG, "No touch device found");
        return;
    }

    // Stop LVGL first (because it's currently using the drivers we want to use)
    tt_lvgl_stop();

    ESP_LOGI(TAG, "Creating display driver");
    auto display = new DisplayDriver(display_id);

    ESP_LOGI(TAG, "Creating display driver");
    auto touch = new TouchDriver(touch_id);

    // Run the main logic
    ESP_LOGI(TAG, "Running application");
    runApplication(display, touch);

    ESP_LOGI(TAG, "Cleanup display driver");
    delete display;

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
