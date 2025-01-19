#pragma once

#include "./App.h"
#include "./AppManifest.h"

namespace tt::app {

class AppCompatC : public App {

public:

    typedef void (*OnStart)(AppContext& app);
    typedef void (*OnStop)(AppContext& app);
    typedef void (*OnShow)(AppContext& app, lv_obj_t* parent);
    typedef void (*OnHide)(AppContext& app);
    typedef void (*OnResult)(AppContext& app, Result result, const Bundle& resultData);

    struct Definition {
        OnStart onStart;
        OnStop onStop;
        OnShow onShow;
        OnHide onHide;
        OnResult onResult;
    };

private:

    const Definition& definition;

public:

    explicit AppCompatC(Definition& definition) : definition(definition) {}

    void onStart(AppContext& appContext) override {
        definition.onStart(appContext);
    }

    void onStop(AppContext& appContext) override {
        definition.onStop(appContext);
    }

    void onShow(AppContext& appContext, lv_obj_t* parent) override {
        definition.onShow(appContext, parent);
    }

    void onHide(AppContext& appContext) override {
        definition.onHide(appContext);
    }

    void onResult(AppContext& appContext, Result result, const Bundle& resultData) override {
        definition.onResult(appContext, result, resultData);
    }
};

}
