#include "nanobake.h"
#include "record.h"
#include "apps/services/loader/loader.h"

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

    FURI_RECORD_TRANSACTION(RECORD_LOADER, Loader*, loader, {
        FuriString* error_message = furi_string_alloc();
        if (loader_start(loader, hello_world_app.id, NULL, error_message) != LoaderStatusOk) {
            FURI_LOG_E(hello_world_app.id, "%s\r\n", furi_string_get_cstr(error_message));
        }
    });
}
