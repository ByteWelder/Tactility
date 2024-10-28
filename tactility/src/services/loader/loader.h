#pragma once

#include "app_manifest.h"
#include "bundle.h"
#include "pubsub.h"
#include "service_manifest.h"
#include "tactility_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Loader Loader;

typedef enum {
    LoaderStatusOk,
    LoaderStatusErrorAppStarted,
    LoaderStatusErrorUnknownApp,
    LoaderStatusErrorInternal,
} LoaderStatus;

typedef enum {
    LoaderEventTypeApplicationStarted,
    LoaderEventTypeApplicationShowing,
    LoaderEventTypeApplicationHiding,
    LoaderEventTypeApplicationStopped
} LoaderEventType;

typedef struct {
    App* app;
} LoaderEventAppStarted;

typedef struct {
    App* app;
} LoaderEventAppShowing;

typedef struct {
    App* app;
} LoaderEventAppHiding;

typedef struct {
    const AppManifest* manifest;
} LoaderEventAppStopped;

typedef struct {
    LoaderEventType type;
    union {
        LoaderEventAppStarted app_started;
        LoaderEventAppShowing app_showing;
        LoaderEventAppHiding app_hiding;
        LoaderEventAppStopped app_stopped;
    };
} LoaderEvent;

/**
 * @brief Start an app
 * @param[in] id application name or id
 * @param[in] blocking application arguments
 * @param[in] bundle optional bundle. Ownership is transferred to Loader.
 * @return LoaderStatus
 */
LoaderStatus loader_start_app(const char* id, bool blocking, Bundle* _Nullable bundle);

/**
 * @brief Stop the currently showing app. Show the previous app if any app was still running.
 */
void loader_stop_app();

App _Nullable loader_get_current_app();

/**
 * @brief PubSub for LoaderEvent
 */
PubSub* loader_get_pubsub();

#ifdef __cplusplus
}
#endif