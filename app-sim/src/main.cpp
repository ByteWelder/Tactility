#include "tactility.h"

extern const tt::HardwareConfig sim_hardware;
extern const tt::AppManifest hello_world_app;

void app_main() {
    static const tt::Config config = {
        .hardware = &sim_hardware,
        .apps = {
            &hello_world_app
        },
        .services = {},
        .auto_start_app_id = nullptr
    };

    tt::init(&config);
}
