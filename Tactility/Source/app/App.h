#pragma once

#include "AppContext.h"
#include "Bundle.h"
#include <Mutex.h>

// Forward declarations
typedef struct _lv_obj_t lv_obj_t;

namespace tt::app {

// Forward declarations
class AppContext;
enum class Result;

class App {

private:

    Mutex mutex;

    struct ResultHolder {
        Result result;
        std::unique_ptr<Bundle> resultData;

        explicit ResultHolder(Result result) : result(result), resultData(nullptr) {}

        ResultHolder(Result result, std::unique_ptr<Bundle> resultData) :
            result(result),
            resultData(std::move(resultData)) {}
    };

    std::unique_ptr<ResultHolder> resultHolder;

public:

    App() = default;
    virtual ~App() = default;

    virtual void onStart(AppContext& appContext) {}
    virtual void onStop(AppContext& appContext) {}
    virtual void onShow(AppContext& appContext, lv_obj_t* parent) {}
    virtual void onHide(AppContext& appContext) {}
    virtual void onResult(AppContext& appContext, Result result, std::unique_ptr<Bundle> _Nullable resultData) {}

    Mutex& getMutex() { return mutex; }

    bool hasResult() const { return resultHolder != nullptr; }

    void setResult(Result result, std::unique_ptr<Bundle> resultData = nullptr) {
        auto lockable = getMutex().scoped();
        lockable->lock(portMAX_DELAY);
        resultHolder = std::make_unique<ResultHolder>(result, std::move(resultData));
    }

    /**
     * Used by system to extract the result data when this application is finished.
     * Note that this removes the data from the class!
     */
    bool moveResult(Result& outResult, std::unique_ptr<Bundle>& outBundle) {
        auto lockable = getMutex().scoped();
        lockable->lock(portMAX_DELAY);

        if (resultHolder != nullptr) {
            outResult = resultHolder->result;
            outBundle = std::move(resultHolder->resultData);
            resultHolder = nullptr;
            return true;
        } else {
            return false;
        }
    }
};

template<typename T>
std::shared_ptr<App> create() { return std::shared_ptr<T>(new T); }

}
