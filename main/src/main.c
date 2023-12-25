#include <nanobake.h>

// Hardware
#include <board_2432s024_touch.h>
#include <board_2432s024_display.h>

// Apps
#include "hello_world/hello_world.h"

void app_main(void) {
    static nb_platform_config_t platform_config = {
        .display_driver = &board_2432s024_create_display_driver,
        .touch_driver = &board_2432s024_create_touch_driver,
        .apps = {
            &hello_world_app
        }
    };

    nanobake_run(&platform_config);
}
