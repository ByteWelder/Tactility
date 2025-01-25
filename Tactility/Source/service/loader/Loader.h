#pragma once

#include "Bundle.h"
#include "PubSub.h"
#include "app/AppManifest.h"
#include "service/ServiceManifest.h"
#include <memory>

namespace tt::service::loader {

// region LoaderEvent for PubSub

typedef enum {
    LoaderEventTypeApplicationStarted,
    LoaderEventTypeApplicationShowing,
    LoaderEventTypeApplicationHiding,
    LoaderEventTypeApplicationStopped
} LoaderEventType;

struct LoaderEvent {
    LoaderEventType type;
};

// endregion LoaderEvent for PubSub

/**
 * @brief Start an app
 * @param[in] id application name or id
 * @param[in] parameters optional parameters to pass onto the application
 */
void startApp(const std::string& id, std::shared_ptr<const Bundle> _Nullable parameters = nullptr);

/** @brief Stop the currently showing app. Show the previous app if any app was still running. */
void stopApp();

/** @return the currently running app context (it is only ever null before the splash screen is shown) */
std::shared_ptr<app::AppContext> _Nullable getCurrentAppContext();

/** @return the currently running app (it is only ever null before the splash screen is shown) */
std::shared_ptr<app::App> _Nullable getCurrentApp();

/**
 * @brief PubSub for LoaderEvent
 */
std::shared_ptr<PubSub> getPubsub();

} // namespace
