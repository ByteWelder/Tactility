#pragma once

#include "nb_app.h"
#include "nb_display.h"
#include "nb_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef NbTouchDriver (*CreateTouchDriver)();
typedef NbDisplayDriver (*CreateDisplayDriver)();

typedef struct {
    // Required driver for display
    const CreateDisplayDriver _Nonnull display_driver;
    // Optional driver for touch input
    const CreateTouchDriver _Nullable touch_driver;
    // List of user applications
    const size_t apps_count;
    const NbApp* const apps[];
} NbConfig;

#ifdef __cplusplus
}
#endif
