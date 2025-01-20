#pragma once

#include "./App.h"
#include "./AppManifest.h"

namespace tt::app {

typedef void* (*CreateData)();
typedef void (*DestroyData)(void* data);
typedef void (*OnStart)(AppContext& app, void* _Nullable data);
typedef void (*OnStop)(AppContext& app, void* _Nullable data);
typedef void (*OnShow)(AppContext& app, void* _Nullable data, lv_obj_t* parent);
typedef void (*OnHide)(AppContext& app, void* _Nullable data);
typedef void (*OnResult)(AppContext& app, void* _Nullable data, Result result, const Bundle& resultData);

class AppCompatC : public App {

private:

    CreateData _Nullable createData;
    DestroyData _Nullable destroyData;
    OnStart _Nullable onStartCallback;
    OnStop _Nullable onStopCallback;
    OnShow _Nullable onShowCallback;
    OnHide _Nullable onHideCallback;
    OnResult _Nullable onResultCallback;

    void* data = nullptr;

public:

    AppCompatC(
        CreateData _Nullable createData,
        DestroyData _Nullable destroyData,
        OnStart _Nullable onStart,
        OnStop _Nullable onStop,
        OnShow _Nullable onShow,
        OnHide _Nullable onHide,
        OnResult _Nullable onResult
    ) : createData(createData),
        destroyData(destroyData),
        onStartCallback(onStart),
        onStopCallback(onStop),
        onShowCallback(onShow),
        onHideCallback(onHide),
        onResultCallback(onResult)
    {}

    void onStart(AppContext& appContext) override {
        if (createData != nullptr) {
            data = createData();
        }

        if (onStartCallback != nullptr) {
            onStartCallback(appContext, data);
        }
    }

    void onStop(AppContext& appContext) override {
        if (onStopCallback != nullptr) {
            onStopCallback(appContext, data);
        }

        if (destroyData != nullptr && data != nullptr) {
            destroyData(data);
        }
    }

    void onShow(AppContext& appContext, lv_obj_t* parent) override {
        if (onShowCallback != nullptr) {
            onShowCallback(appContext, data, parent);
        }
    }

    void onHide(AppContext& appContext) override {
        if (onHideCallback != nullptr) {
            onHideCallback(appContext, data);
        }
    }

    void onResult(AppContext& appContext, Result result, const Bundle& resultData) override {
        if (onResultCallback != nullptr) {
            onResultCallback(appContext, data, result, resultData);
        }
    }
};

template<typename T>
App* createC(
    CreateData _Nullable createData,
    DestroyData _Nullable destroyData,
    OnStart _Nullable onStartCallback,
    OnStop _Nullable onStopCallback,
    OnShow _Nullable onShowCallback,
    OnHide _Nullable onHideCallback,
    OnResult _Nullable onResultCallback
) {
    return new AppCompatC(
        createData,
        destroyData,
        onStartCallback,
        onStopCallback,
        onShowCallback,
        onHideCallback,
        onResultCallback
    );
}

}
