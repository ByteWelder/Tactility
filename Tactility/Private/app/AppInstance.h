#pragma once

#include "app/AppContext.h"
#include "app/AppManifest.h"
#include "Bundle.h"
#include "Mutex.h"
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
    explicit ResultHolder(Result result) {
        this->result = result;
        this->resultData = nullptr;
    }

    ResultHolder(Result result, const Bundle& resultData) {
        this->result = result;
        this->resultData = std::make_unique<Bundle>(resultData);
    }

    Result result;
    std::unique_ptr<Bundle> resultData;
};

/**
 * Thread-safe app instance.
 */
class AppInstance : public AppContext {

private:

    Mutex mutex = Mutex(MutexTypeNormal);
    const AppManifest& manifest;
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

public:

    explicit AppInstance(const AppManifest& manifest) :
        manifest(manifest) {}

    AppInstance(const AppManifest& manifest, std::shared_ptr<const Bundle> parameters) :
        manifest(manifest),
        parameters(std::move(parameters)) {}

    ~AppInstance() override = default;

    void setState(State state);
    [[nodiscard]] State getState() const;

    [[nodiscard]] const AppManifest& getManifest() const override;

    [[nodiscard]] Flags getFlags() const override;
    void setFlags(Flags flags);
    Flags& mutableFlags() { return flags; } // TODO: locking mechanism

    [[nodiscard]] std::shared_ptr<void> _Nullable getData() const override;
    void setData(std::shared_ptr<void> data) override;

    [[nodiscard]] std::shared_ptr<const Bundle> getParameters() const override;

    void setResult(Result result) override;
    void setResult(Result result, const Bundle& bundle) override;
    [[nodiscard]] bool hasResult() const override;
    std::unique_ptr<ResultHolder>& getResult() { return resultHolder; }
};

} // namespace
