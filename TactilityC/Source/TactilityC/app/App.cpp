#include <Check.h>
#include "App.h"
#include "Log.h"
#include "app/ElfApp.h"

#define TAG "tactilityc_app"

AppOnStart elfOnStart = nullptr;
AppOnStop elfOnStop = nullptr;
AppOnShow elfOnShow = nullptr;
AppOnHide elfOnHide = nullptr;
AppOnResult elfOnResult = nullptr;

static void onStartWrapper(tt::app::AppContext& context) {
    if (elfOnStart != nullptr) {
        TT_LOG_I(TAG, "onStartWrapper");
        elfOnStart(&context);
    } else {
        TT_LOG_W(TAG, "onStartWrapper not set");
    }
}

static void onStopWrapper(tt::app::AppContext& context) {
    if (elfOnStop != nullptr) {
        TT_LOG_I(TAG, "onStopWrapper");
        elfOnStop(&context);
    } else {
        TT_LOG_W(TAG, "onStopWrapper not set");
    }
}

static void onShowWrapper(tt::app::AppContext& context, lv_obj_t* parent) {
    if (elfOnShow != nullptr) {
        TT_LOG_I(TAG, "onShowWrapper");
        elfOnShow(&context, parent);
    } else {
        TT_LOG_W(TAG, "onShowWrapper not set");
    }
}

static void onHideWrapper(tt::app::AppContext& context) {
    if (elfOnHide != nullptr) {
        TT_LOG_I(TAG, "onHideWrapper");
        elfOnHide(&context);
    } else {
        TT_LOG_W(TAG, "onHideWrapper not set");
    }
}

static void onResultWrapper(tt::app::AppContext& context, tt::app::Result result, const tt::Bundle& resultData) {
    if (elfOnResult != nullptr) {
        TT_LOG_I(TAG, "onResultWrapper");
        Result convertedResult = AppResultError;
        switch (result) {
            case tt::app::ResultOk:
                convertedResult = AppResultOk;
                break;
            case tt::app::ResultCancelled:
                convertedResult = AppResultCancelled;
                break;
            case tt::app::ResultError:
                convertedResult = AppResultError;
                break;
        }
        elfOnResult(&context, convertedResult, (BundleHandle)&resultData);
    } else {
        TT_LOG_W(TAG, "onResultWrapper not set");
    }
}

tt::app::AppManifest manifest = {
    .id = "ElfWrapperInTactilityC",
    .name = "",
    .icon = "",
    .onStart = onStartWrapper,
    .onStop = onStopWrapper,
    .onShow = onShowWrapper,
    .onHide = onHideWrapper,
    .onResult = onResultWrapper
};

extern "C" {

void tt_set_app_manifest(
    const char* name,
    const char* _Nullable icon,
    AppOnStart onStart,
    AppOnStop _Nullable onStop,
    AppOnShow _Nullable onShow,
    AppOnHide _Nullable onHide,
    AppOnResult _Nullable onResult
) {
#ifdef ESP_PLATFORM
    manifest.name = name;
    manifest.icon = icon ? icon : "";
    elfOnStart = onStart;
    elfOnStop = onStop;
    elfOnShow = onShow;
    elfOnHide = onHide;
    elfOnResult = onResult;
    tt::app::setElfAppManifest(manifest);
#else
    tt_crash("Not intended for PC");
#endif
}

}