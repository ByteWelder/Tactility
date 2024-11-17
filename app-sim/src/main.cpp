#include "hello_world/hello_world.h"
#include "tactility.h"

extern const HardwareConfig sim_hardware;

void app_main() {
    static const Config config = {
        .hardware = &sim_hardware,
        .apps = {
            &hello_world_app
        },
        .services = {},
        .auto_start_app_id = nullptr
    };

    tt_init(&config);
}