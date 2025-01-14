#pragma once

#include "app/AppManifest.h"
#include "Bundle.h"
#include "Pubsub.h"
#include "service/ServiceManifest.h"
#include <memory>

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
 * @param[in] parameters optional parameters to pass onto the application
 */
void startApp(const std::string& id, const std::shared_ptr<const Bundle>& _Nullable parameters = nullptr);

/** @brief Stop the currently showing app. Show the previous app if any app was still running. */
void stopApp();

/** @return the currently running app (it is only ever null before the splash screen is shown) */
app::AppContext* _Nullable getCurrentApp();

/**
 * @brief PubSub for LoaderEvent
 */
std::shared_ptr<PubSub> getPubsub();

} // namespace
