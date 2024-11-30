#pragma once

#include "app/Manifest.h"
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


/**
 * @brief Start an app
 * @param[in] id application name or id
 * @param[in] blocking whether this call is blocking or not. You cannot call this from an LVGL thread.
 * @param[in] arguments optional parameters to pass onto the application
 * @return LoaderStatus
 */
LoaderStatus startApp(const std::string& id, bool blocking = false, const Bundle& arguments = Bundle());

/**
 * @brief Stop the currently showing app. Show the previous app if any app was still running.
 */
void stopApp();

app::App* _Nullable getCurrentApp();

/**
 * @brief PubSub for LoaderEvent
 */
PubSub* getPubsub();

} // namespace
