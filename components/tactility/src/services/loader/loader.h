#pragma once

#include "app_manifest.h"
#include "bundle.h"
#include "furi_core.h"
#include "furi_string.h"
#include "pubsub.h"
#include "service_manifest.h"

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
    LoaderEventTypeApplicationStopped
} LoaderEventType;

typedef struct {
    LoaderEventType type;
} LoaderEvent;

/**
 * @brief Close any running app, then start new one. Blocking.
 * @param[in] id application name or id
 * @param[in] blocking application arguments
 * @return LoaderStatus
 */
LoaderStatus loader_start_app(const char* id, bool blocking, Bundle* _Nullable bundle);

void loader_stop_app();

App _Nullable loader_get_current_app();

/**
 * @brief Get loader pubsub
 * @return FuriPubSub*
 */
FuriPubSub* loader_get_pubsub();

#ifdef __cplusplus
}
#endif