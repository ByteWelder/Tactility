#pragma once

#include "Tactility/app/AppManifest.h"

#include <Tactility/Bundle.h>
#include <Tactility/PubSub.h>

#include <memory>

namespace tt::service::loader {

// region LoaderEvent for PubSub

enum class LoaderEvent{
    ApplicationStarted,
    ApplicationShowing,
    ApplicationHiding,
    ApplicationStopped
};

// endregion LoaderEvent for PubSub

/**
 * @brief Start an app
 * @param[in] id application name or id
 * @param[in] parameters optional parameters to pass onto the application
 * @return the launch id
 */
app::LaunchId startApp(const std::string& id, std::shared_ptr<const Bundle> _Nullable parameters = nullptr);

/** @brief Stop the currently showing app. Show the previous app if any app was still running. */
void stopApp();

/** @return the currently running app context (it is only ever null before the splash screen is shown) */
std::shared_ptr<app::AppContext> _Nullable getCurrentAppContext();

/** @return the currently running app (it is only ever null before the splash screen is shown) */
std::shared_ptr<app::App> _Nullable getCurrentApp();

/**
 * @brief PubSub for LoaderEvent
 */
std::shared_ptr<PubSub<LoaderEvent>> getPubsub();

} // namespace
