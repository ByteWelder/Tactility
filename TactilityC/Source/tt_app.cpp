#include "tt_app.h"
#include <Tactility/app/App.h>
#include <Tactility/app/AppPaths.h>
#include <Tactility/app/AppContext.h>

extern "C" {

constexpr auto* TAG = "tt_app";

#define HANDLE_AS_APP_CONTEXT(handle) ((tt::app::AppContext*)(handle))

BundleHandle _Nullable tt_app_get_parameters(AppHandle handle) {
    return (BundleHandle)HANDLE_AS_APP_CONTEXT(handle)->getParameters().get();
}

void tt_app_set_result(AppHandle handle, AppResult result, BundleHandle _Nullable bundle) {
    auto shared_bundle = std::unique_ptr<tt::Bundle>(static_cast<tt::Bundle*>(bundle));
    HANDLE_AS_APP_CONTEXT(handle)->getApp()->setResult(static_cast<tt::app::Result>(result), std::move(shared_bundle));
}

bool tt_app_has_result(AppHandle handle) {
    return HANDLE_AS_APP_CONTEXT(handle)->getApp()->hasResult();
}

void tt_app_start(const char* appId) {
    tt::app::start(appId);
}

void tt_app_start_with_bundle(const char* appId, BundleHandle parameters) {
    tt::app::start(appId, std::shared_ptr<tt::Bundle>(static_cast<tt::Bundle*>(parameters)));
}

void tt_app_stop() {
    tt::app::stop();
}

void tt_app_get_data_directory(AppPathsHandle handle, char* buffer, size_t* size) {
    assert(buffer != nullptr);
    assert(size != nullptr);
    assert(*size > 0);
    auto paths = HANDLE_AS_APP_CONTEXT(handle)->getPaths();
    auto data_path = paths->getUserDataPath();
    auto expected_length = data_path.length() + 1;
    if (*size < expected_length) {
        TT_LOG_E(TAG, "Path buffer not large enough (%d < %d)", *size, expected_length);
        *size = 0;
        buffer[0] = 0;
        return;
    }

    strcpy(buffer, data_path.c_str());
    *size = data_path.length();
}

}