#include "tt_app_context.h"
#include <app/AppContext.h>

struct AppContextDataWrapper {
    void* _Nullable data;
};

extern "C" {

#define HANDLE_AS_APP_CONTEXT(handle) ((tt::app::AppContext*)(handle))

void* _Nullable tt_app_context_get_data(AppContextHandle handle) {
    auto wrapper = std::reinterpret_pointer_cast<AppContextDataWrapper>(HANDLE_AS_APP_CONTEXT(handle)->getData());
    return wrapper ? wrapper->data : nullptr;
}

void tt_app_context_set_data(AppContextHandle handle, void* _Nullable data) {
    auto wrapper = std::make_shared<AppContextDataWrapper>();
    wrapper->data = data;
    HANDLE_AS_APP_CONTEXT(handle)->setData(std::move(wrapper));
}

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