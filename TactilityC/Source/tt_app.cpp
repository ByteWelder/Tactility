#include "tt_app.h"
#include <app/App.h>
#include <app/AppContext.h>

extern "C" {

#define HANDLE_AS_APP_CONTEXT(handle) ((tt::app::AppContext*)(handle))

BundleHandle _Nullable tt_app_get_parameters(AppHandle handle) {
    return (BundleHandle)HANDLE_AS_APP_CONTEXT(handle)->getParameters().get();
}

void tt_app_set_result(AppHandle handle, Result result, BundleHandle _Nullable bundle) {
    auto shared_bundle = std::unique_ptr<tt::Bundle>((tt::Bundle*)bundle);
    HANDLE_AS_APP_CONTEXT(handle)->getApp()->setResult((tt::app::Result)result, std::move(shared_bundle));
}

bool tt_app_has_result(AppHandle handle) {
    return HANDLE_AS_APP_CONTEXT(handle)->getApp()->hasResult();
}

void tt_app_start(const char* appId) {
    tt::app::start(appId);
}

void tt_app_start_with_bundle(const char* appId, BundleHandle parameters) {
    tt::app::start(appId, std::shared_ptr<tt::Bundle>((tt::Bundle*)parameters));
}

}