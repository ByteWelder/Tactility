#pragma once

#include "./App.h"
#include "./AppManifest.h"

namespace tt::app {

typedef void (*OnStart)(AppContext& app);
typedef void (*OnStop)(AppContext& app);
typedef void (*OnShow)(AppContext& app, lv_obj_t* parent);
typedef void (*OnHide)(AppContext& app);
typedef void (*OnResult)(AppContext& app, Result result, const Bundle& resultData);

class AppCompatC : public App {

private:

    OnStart _Nullable onStartCallback;
    OnStop _Nullable onStopCallback;
    OnShow _Nullable onShowCallback;
    OnHide _Nullable onHideCallback;
    OnResult _Nullable onResultCallback;

public:

    AppCompatC(
        OnStart _Nullable onStart,
        OnStop _Nullable onStop,
        OnShow _Nullable onShow,
        OnHide _Nullable onHide,
        OnResult _Nullable onResult
    ) : onStartCallback(onStart),
        onStopCallback(onStop),
        onShowCallback(onShow),
        onHideCallback(onHide),
        onResultCallback(onResult)
    {}

    void onStart(AppContext& appContext) override {
        if (onStartCallback != nullptr) {
            onStartCallback(appContext);
        }
    }

    void onStop(AppContext& appContext) override {
        if (onStopCallback != nullptr) {
            onStopCallback(appContext);
        }
    }

    void onShow(AppContext& appContext, lv_obj_t* parent) override {
        if (onShowCallback != nullptr) {
            onShowCallback(appContext, parent);
        }
    }

    void onHide(AppContext& appContext) override {
        if (onHideCallback != nullptr) {
            onHideCallback(appContext);
        }
    }

    void onResult(AppContext& appContext, Result result, const Bundle& resultData) override {
        if (onResultCallback != nullptr) {
            onResultCallback(appContext, result, resultData);
        }
    }
};

template<typename T>
App* createC(
    OnStart _Nullable onStartCallback,
    OnStop _Nullable onStopCallback,
    OnShow _Nullable onShowCallback,
    OnHide _Nullable onHideCallback,
    OnResult _Nullable onResultCallback
) {
    return new AppCompatC(
        onStartCallback,
        onStopCallback,
        onShowCallback,
        onHideCallback,
        onResultCallback
    );
}

}
