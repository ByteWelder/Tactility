#include "Application.h"
#include "Drivers.h"

#include <tt_app.h>
#include <tt_lvgl.h>

static void onCreate(AppHandle appHandle, void* data) {
    Drivers drivers;

    // Find the hardware devices and verify support for the driver interface that we need
    // Not all graphics and touch drivers support accessing them directly
    if (!drivers.validateSupport()) {
        tt_app_stop();
        return;
    }

    // Stop LVGL first (because it's currently using the drivers we want to use)
    tt_lvgl_stop();

    // Start using the drivers
    if (drivers.start()) {
        // Run the main logic
        runApplication(drivers.display, drivers.touch);
        // Stop the drivers
        drivers.stop();
    }

    tt_app_stop();
}

static void onDestroy(AppHandle appHandle, void* data) {
    // Restart LVGL to resume rendering of regular apps
    if (!tt_lvgl_is_started()) {
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
