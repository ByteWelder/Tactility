#include "nanobake.h"
#include "board_config.h"

// Apps
#include "hello_world/hello_world.h"

__attribute__((unused)) void app_main(void) {
    static const Config config = {
        .hardware = NB_BOARD_HARDWARE,
        .apps = {
            &hello_world_app
        },
        .services = { },
    };

    nanobake_start(&config);
}
