#pragma once

#include "nb_display.h"
#include "nb_touch.h"
#include "nb_app.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef nb_touch_driver_t (*create_touch_driver)();
typedef nb_display_driver_t (*create_display_driver)();

typedef struct nb_config nb_config_t;
struct nb_config {
    // Required driver for display
    const create_display_driver _Nonnull display_driver;
    // Optional driver for touch input
    const create_touch_driver _Nullable touch_driver;
    // List of user applications
    const size_t apps_count;
    const nb_app_t* const apps[];
};

#ifdef __cplusplus
}
#endif
