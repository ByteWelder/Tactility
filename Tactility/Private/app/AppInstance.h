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

struct ResultHolder {
    Result result;
    std::shared_ptr<const Bundle> resultData;

    explicit ResultHolder(Result result) : result(result), resultData(nullptr) {}

    ResultHolder(Result result, std::shared_ptr<const Bundle> resultData) :
        result(result),
        resultData(std::move(resultData))
    {}

};

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
    /** @brief @brief Contextual data related to the running app's instance
     * The app can attach its data to this.
     * The lifecycle is determined by the on_start and on_stop methods in the AppManifest.
     * These manifest methods can optionally allocate/free data that is attached here.
     */
    std::shared_ptr<void> _Nullable data;
    std::unique_ptr<ResultHolder> _Nullable resultHolder;

    std::shared_ptr<App> app;

    static std::shared_ptr<app::App> createApp(
        const std::shared_ptr<app::AppManifest>& manifest
    ) {
        if (manifest->location.isInternal()) {
            tt_assert(manifest->createApp != nullptr);
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

    std::shared_ptr<void> _Nullable getData() const override;
    void setData(std::shared_ptr<void> data) override;

    std::shared_ptr<const Bundle> getParameters() const override;

    void setResult(Result result) override;
    void setResult(Result result, std::shared_ptr<const Bundle> bundle) override;
    bool hasResult() const override;

    std::unique_ptr<Paths> getPaths() const override;

    std::unique_ptr<ResultHolder>& getResult() { return resultHolder; }

    const std::shared_ptr<App>& getApp() const { return app; }
};

} // namespace
