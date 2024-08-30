#include "board_config.h"

// Apps
#include "hello_world/hello_world.h"
#include "tactility.h"

extern void wifi_main(void*);

void app_main(void) {
    static const Config config = {
        /**
         * Auto-select a board based on the ./sdkconfig.board.* file
         * that you copied to ./sdkconfig before you opened this project.
         */
        .hardware = TT_BOARD_HARDWARE,
        .apps = {
            &hello_world_app,
        },
        .services = {},
        .auto_start_app_id = NULL
    };

    tt_init(&config);

    wifi_main(NULL);
}
