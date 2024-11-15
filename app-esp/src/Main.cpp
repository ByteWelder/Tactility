#include "Boards.h"

// Apps
#include "hello_world/hello_world.h"
#include "tactility.h"

extern void wifi_main(void*);

extern "C" {

void app_main() {
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
        .auto_start_app_id = nullptr
    };

    tt_init(&config);

    wifi_main(nullptr);
}

}
