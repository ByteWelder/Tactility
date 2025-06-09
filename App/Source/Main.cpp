#include "Boards.h"
#include <Tactility/Tactility.h>

#ifdef ESP_PLATFORM
#include "tt_init.h"
#endif

// Forward declaration of clock_app from Clock.cpp
namespace tt::app::clock {
    extern const AppManifest clock_app;
}

// extern const tt::app::AppManifest hello_world_app;
extern const tt::app::AppManifest calibration_app;
extern const tt::app::AppManifest tactility_news_app;
extern const tt::app::AppManifest tactile_web_app;
extern const tt::app::AppManifest tactiligotchi_app;

extern "C" {

void app_main() {
    static const tt::Configuration config = {
        /**
         * Auto-select a board based on the ./sdkconfig.board.* file
         * that you copied to ./sdkconfig before you opened this project.
         */
        .hardware = TT_BOARD_HARDWARE,
        .apps = {
            // &hello_world_app,
            &calibration_app,
            &tactility_news_app,
            &tactile_web_app,
            &tt::app::clock::clock_app,
            &tactiligotchi_app,
        }
    };

#ifdef ESP_PLATFORM
    tt_init_tactility_c(); // ELF bindings for side-loading on ESP32
#endif

    tt::run(config);
}

} // extern "C"
