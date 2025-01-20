#pragma once

#include "AppContext.h"
#include "Bundle.h"

// Forward declarations
typedef struct _lv_obj_t lv_obj_t;

namespace tt::app {

// Forward declarations
class AppContext;
enum class Result;

class App {
public:
    App() = default;
    virtual ~App() = default;

    virtual void onStart(AppContext& appContext) {}
    virtual void onStop(AppContext& appContext) {}
    virtual void onShow(AppContext& appContext, lv_obj_t* parent) {}
    virtual void onHide(AppContext& appContext) {}
    virtual void onResult(AppContext& appContext, Result result, const Bundle& resultData) {}
};

template<typename T>
std::shared_ptr<App> create() { return std::shared_ptr<T>(new T); }

}
