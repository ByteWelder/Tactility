#include "app/files/FilesPrivate.h"

#include "app/AppContext.h"
#include "Assets.h"
#include "service/loader/Loader.h"

#include <memory>

namespace tt::app::files {

#define TAG "files_app"

extern const AppManifest manifest;

/** Returns the app data if the app is active. Note that this could clash if the same app is started twice and a background thread is slow. */

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto files = std::static_pointer_cast<Files>(app.getData());
    files->onShow(parent);
}

static void onStart(AppContext& app) {
    auto files = std::make_shared<Files>();
    app.setData(files);
}

static void onResult(AppContext& app, Result result, const Bundle& bundle) {
    auto files = std::static_pointer_cast<Files>(app.getData());
    files->onResult(result, bundle);
}

extern const AppManifest manifest = {
    .id = "Files",
    .name = "Files",
    .icon = TT_ASSETS_APP_ICON_FILES,
    .type = TypeHidden,
    .onStart = onStart,
    .onShow = onShow,
    .onResult = onResult
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
