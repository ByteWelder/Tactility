#pragma once

#include "Bundle.h"
#include "Mutex.h"
#include "app/AppContext.h"
#include "app/AppManifest.h"
#include "app/ElfApp.h"
#include <memory>
#include <utility>

namespace tt::app {

typedef enum {
    StateInitial, // App is being activated in loader
    StateStarted, // App is in memory
    StateShowing, // App view is created
    StateHiding,  // App view is destroyed
    StateStopped  // App is not in memory
} State;

/**
 * Thread-safe app instance.
 */
class AppInstance : public AppContext {

private:

    Mutex mutex = Mutex(Mutex::Type::Normal);
    const std::shared_ptr<AppManifest> manifest;
    State state = StateInitial;
    Flags flags = { .showStatusbar = true };
    /** @brief Optional parameters to start the app with
     * When these are stored in the app struct, the struct takes ownership.
     * Do not mutate after app creation.
     */
    std::shared_ptr<const tt::Bundle> _Nullable parameters;

    std::shared_ptr<App> app;

    static std::shared_ptr<app::App> createApp(
        const std::shared_ptr<app::AppManifest>& manifest
    ) {
        if (manifest->location.isInternal()) {
            assert(manifest->createApp != nullptr);
            return manifest->createApp();
        } else if (manifest->location.isExternal()) {
            if (manifest->createApp != nullptr) {
                TT_LOG_W("", "Manifest specifies createApp, but this is not used with external apps");
            }
#ifdef ESP_PLATFORM
            return app::createElfApp(manifest);
#else
            tt_crash("not supported");
#endif
        } else {
            tt_crash("not implemented");
        }
    }

public:

    explicit AppInstance(const std::shared_ptr<AppManifest>& manifest) :
        manifest(manifest),
        app(createApp(manifest))
    {}

    AppInstance(const std::shared_ptr<AppManifest>& manifest, std::shared_ptr<const Bundle> parameters) :
        manifest(manifest),
        parameters(std::move(parameters)),
        app(createApp(manifest)) {}

    ~AppInstance() override = default;

    void setState(State state);
    State getState() const;

    const AppManifest& getManifest() const override;

    Flags getFlags() const;
    void setFlags(Flags flags);
    Flags& mutableFlags() { return flags; } // TODO: locking mechanism

    std::shared_ptr<const Bundle> getParameters() const override;

    std::unique_ptr<Paths> getPaths() const override;

    std::shared_ptr<App> getApp() const override { return app; }
};

} // namespace
