#include "Boards.h"

// Apps
#include "Tactility.h"

namespace tt::service::wifi {
    extern void wifi_main(void*);
}

extern const tt::app::AppManifest hello_world_app;

extern "C" {

void app_main() {
    static const tt::Configuration config = {
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

    tt::init(config);

    tt::service::wifi::wifi_main(nullptr);
}

} // extern
