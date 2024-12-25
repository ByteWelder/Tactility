#include "Boards.h"

// Apps
#include "Tactility.h"
#include "tt_init.h"

namespace tt::service::wifi {
    extern void wifi_task(void*);
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
        .autoStartAppId = nullptr
    };

    tt_init_tactility_c(); // ELF bindings for side-loading on ESP32

    tt::run(config);
}

} // extern
