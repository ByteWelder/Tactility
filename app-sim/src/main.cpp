#include "Tactility.h"

extern const tt::hal::Configuration sim_hardware;
extern const tt::AppManifest hello_world_app;

void app_main() {
    static const tt::Configuration config = {
        .hardware = &sim_hardware,
        .apps = {
            &hello_world_app
        },
        .services = {},
        .auto_start_app_id = nullptr
    };

    tt::init(&config);
}
