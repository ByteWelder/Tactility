#pragma once

#include "app_manifest.h"
#include "context.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_STATE_INITIAL, // App is being activated in loader
    APP_STATE_STARTED, // App is in memory
    APP_STATE_SHOWING, // App view is created
    APP_STATE_HIDING,  // App view is destroyed
    APP_STATE_STOPPED  // App is not in memory
} AppState;

typedef union {
    struct {
        bool show_statusbar : 1;
        bool show_toolbar : 1;
    };
    unsigned char flags;
} AppFlags;

typedef struct {
    AppState state;
    AppFlags flags;
    const AppManifest* manifest;
    Context context;
} App;

#ifdef __cplusplus
}
#endif
