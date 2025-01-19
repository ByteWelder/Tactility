#include "tt_app_manifest.h"

#include <Check.h>
#include <Log.h>
#include <app/ElfApp.h>
#include <app/AppCompatC.h>

#define TAG "tt_app"

AppOnStart elfOnStart = nullptr;
AppOnStop elfOnStop = nullptr;
AppOnShow elfOnShow = nullptr;
AppOnHide elfOnHide = nullptr;
AppOnResult elfOnResult = nullptr;

static void onResultWrapper(tt::app::AppContext& context, tt::app::Result result, const tt::Bundle& resultData) {
    if (elfOnResult != nullptr) {
        TT_LOG_I(TAG, "onResultWrapper");
        Result convertedResult = AppResultError;
        switch (result) {
            case tt::app::Result::Ok:
                convertedResult = AppResultOk;
                break;
            case tt::app::Result::Cancelled:
                convertedResult = AppResultCancelled;
                break;
            case tt::app::Result::Error:
                convertedResult = AppResultError;
                break;
        }
        elfOnResult(&context, convertedResult, (BundleHandle)&resultData);
    } else {
        TT_LOG_W(TAG, "onResultWrapper not set");
    }
}

extern "C" {

void tt_set_app_manifest(
    const char* name,
    const char* _Nullable icon,
    AppOnStart _Nullable onStart,
    AppOnStop _Nullable onStop,
    AppOnShow _Nullable onShow,
    AppOnHide _Nullable onHide,
    AppOnResult _Nullable onResult
) {
#ifdef ESP_PLATFORM
    tt::app::setElfAppManifest(
        name,
        icon,
        (tt::app::OnStart)onStart,
        (tt::app::OnStop)onStop,
        (tt::app::OnShow)onShow,
        (tt::app::OnHide)onHide,
        (tt::app::OnResult)onResult
    );
#else
    tt_crash("TactilityC is intended for PC/Simulator");
#endif
}

}
