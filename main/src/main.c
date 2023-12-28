#include "nanobake.h"

// Hardware
#include "board_2432s024.h"

// Apps
#include "hello_world/hello_world.h"

__attribute__((unused)) void app_main(void) {
    static Config config = {
        .display_driver = &board_2432s024_create_display_driver,
        .touch_driver = &board_2432s024_create_touch_driver,
        .apps = {
            &hello_world_app
        },
        .apps_count = 1
    };

    nanobake_start(&config);
}
