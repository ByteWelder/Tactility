#pragma once

#include "app.h"
#include "devices.h"
#include "core_defines.h"
#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef void* FuriThreadId;
typedef TouchDriver (*CreateTouchDriver)();
typedef DisplayDriver (*CreateDisplayDriver)();

typedef struct {
    // Required driver for display
    const CreateDisplayDriver _Nonnull display_driver;
    // Optional driver for touch input
    const CreateTouchDriver _Nullable touch_driver;
    // List of user applications
    const size_t apps_count;
    const App* const apps[];
} Config;

__attribute__((unused)) extern void nanobake_start(Config _Nonnull* config);

FuriThreadId nanobake_get_app_thread_id(size_t index);
size_t nanobake_get_app_thread_count();

#ifdef __cplusplus
}
#endif
