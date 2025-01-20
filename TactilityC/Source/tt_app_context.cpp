#include "tt_app_context.h"
#include <app/AppContext.h>

struct AppContextDataWrapper {
    void* _Nullable data;
};

extern "C" {

#define HANDLE_AS_APP_CONTEXT(handle) ((tt::app::AppContext*)(handle))

BundleHandle _Nullable tt_app_context_get_parameters(AppContextHandle handle) {
    return (BundleHandle)HANDLE_AS_APP_CONTEXT(handle)->getParameters().get();
}

void tt_app_context_set_result(AppContextHandle handle, Result result, BundleHandle _Nullable bundle) {
    auto shared_bundle = std::shared_ptr<tt::Bundle>((tt::Bundle*)bundle);
    HANDLE_AS_APP_CONTEXT(handle)->setResult((tt::app::Result)result, std::move(shared_bundle));
}

bool tt_app_context_has_result(AppContextHandle handle) {
    return HANDLE_AS_APP_CONTEXT(handle)->hasResult();
}

}