#pragma once
#include "furi_core.h"
#include "furi_string.h"
#include "pubsub.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_LOADER "loader"

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
 * @param[in] loader loader instance
 * @param[in] id application name or id
 * @param[in] args application arguments
 * @param[out] error_message detailed error message, can be NULL
 * @return LoaderStatus
 */
LoaderStatus loader_start_app(Loader* loader, const char* id, const char* args, FuriString* error_message);

/**
 * @brief Close any running app, then start new one. Non-blocking.
 * @param[in] loader loader instance
 * @param[in] id application name or id
 * @param[in] args application arguments
 */
void loader_start_app_nonblocking(Loader* loader, const char* id, const char* args);

void loader_stop_app(Loader* loader);

/**
 * @brief Start application with GUI error message
 * @param[in] instance loader instance
 * @param[in] name application name or id
 * @param[in] args application arguments
 * @return LoaderStatus
 */
//LoaderStatus loader_start_with_gui_error(Loader* loader, const char* name, const char* args);

/**
 * @brief Show loader menu
 * @param[in] instance loader instance
 */
void loader_show_menu(Loader* instance);

/**
 * @brief Get loader pubsub
 * @param[in] instance loader instance
 * @return FuriPubSub*
 */
FuriPubSub* loader_get_pubsub(Loader* instance);

#ifdef __cplusplus
}
#endif