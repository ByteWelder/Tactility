#pragma once
#include "app_manifest.h"
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
 * @param[in] args application arguments
 * @param[out] error_message detailed error message, can be NULL
 * @return LoaderStatus
 */
LoaderStatus loader_start_app(const char* id, FuriString* error_message);

/**
 * @brief Close any running app, then start new one. Non-blocking.
 * @param[in] id application name or id
 * @param[in] args application arguments
 */
void loader_start_app_nonblocking(const char* id);

void loader_stop_app();

bool loader_is_app_running();

const AppManifest* _Nullable loader_get_current_app();
/**
 * @brief Start application with GUI error message
 * @param[in] name application name or id
 * @param[in] args application arguments
 * @return LoaderStatus
 */
//LoaderStatus loader_start_with_gui_error(const char* name, const char* args);

/**
 * @brief Show loader menu
 */
void loader_show_menu();

/**
 * @brief Get loader pubsub
 * @return FuriPubSub*
 */
FuriPubSub* loader_get_pubsub();

#ifdef __cplusplus
}
#endif