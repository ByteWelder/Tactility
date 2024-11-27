#pragma once

#include "app/App.h"
#include "app/Manifest.h"
#include "Bundle.h"
#include "Mutex.h"
#include <memory>

namespace tt::app {

typedef enum {
    StateInitial, // App is being activated in loader
    StateStarted, // App is in memory
    StateShowing, // App view is created
    StateHiding,  // App view is destroyed
    StateStopped  // App is not in memory
} State;

struct ResultHolder {
    ResultHolder(Result result, const Bundle& resultData) {
        this->result = result;
        this->resultData = new Bundle(resultData);
    }

    ~ResultHolder() {
        delete resultData;
    }

    Result result;
    Bundle* _Nullable resultData;
};

/**
 * Thread-safe app instance.
 */
class AppInstance : public App {

private:

    Mutex mutex = Mutex(MutexTypeNormal);
    const Manifest& manifest;
    State state = StateInitial;
    Flags flags = { .showStatusbar = true };
    /** @brief Optional parameters to start the app with
     * When these are stored in the app struct, the struct takes ownership.
     * Do not mutate after app creation.
     */
    tt::Bundle parameters;
    /** @brief @brief Contextual data related to the running app's instance
     * The app can attach its data to this.
     * The lifecycle is determined by the on_start and on_stop methods in the AppManifest.
     * These manifest methods can optionally allocate/free data that is attached here.
     */
    void* _Nullable data = nullptr;
    std::unique_ptr<ResultHolder> resultHolder;

public:

    AppInstance(const Manifest& manifest) :
        manifest(manifest) {}

    AppInstance(const Manifest& manifest, const Bundle& parameters) :
        manifest(manifest),
        parameters(parameters) {}

    ~AppInstance() {}

    void setState(State state);
    State getState() const;

    const Manifest& getManifest() const;

    Flags getFlags() const;
    void setFlags(Flags flags);

    _Nullable void* getData() const;
    void setData(void* data);

    const Bundle& getParameters() const;

    void setResult(Result result, const Bundle& bundle);
    bool hasResult() const;
    std::unique_ptr<ResultHolder>& getResult() { return resultHolder; }
};

} // namespace
