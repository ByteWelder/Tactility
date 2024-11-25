#pragma once

#include "AppManifest.h"
#include "Bundle.h"
#include "Pubsub.h"
#include "service/Manifest.h"

namespace tt::service::loader {

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
LoaderStatus start_app(const std::string& id, bool blocking, const Bundle& bundle);

/**
 * @brief Stop the currently showing app. Show the previous app if any app was still running.
 */
void stop_app();

App _Nullable get_current_app();

/**
 * @brief PubSub for LoaderEvent
 */
PubSub* get_pubsub();

} // namespace
