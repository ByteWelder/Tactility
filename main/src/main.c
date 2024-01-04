#include "nanobake.h"
#include "board_detect.h"

// Apps
#include "hello_world/hello_world.h"

__attribute__((unused)) void app_main(void) {
    static Config config = {
        .hardware = HARDWARE_AUTO_DETECT,
        .apps = {
            &hello_world_app
        },
        .apps_count = 1
    };

    nanobake_start(&config);
}
