#include "Boards.h"
#include <Tactility/Tactility.h>

#ifdef ESP_PLATFORM
#include "tt_init.h"
#endif

extern const tt::app::AppManifest hello_world_app;

extern "C" {

void app_main() {
    static const tt::Configuration config = {
        /**
         * Auto-select a board based on the ./sdkconfig.board.* file
         * that you copied to ./sdkconfig before you opened this project.
         */
        .hardware = TT_BOARD_HARDWARE
    };

#ifdef ESP_PLATFORM
    tt_init_tactility_c(); // ELF bindings for side-loading on ESP32
#endif

    tt::run(config);
}

} // extern