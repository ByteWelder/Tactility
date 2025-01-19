#include "app/files/FilesPrivate.h"

#include "app/AppContext.h"
#include "Assets.h"
#include "service/loader/Loader.h"

#include <memory>

namespace tt::app::files {

#define TAG "files_app"

extern const AppManifest manifest;

class FilesApp : public App {

    void onShow(AppContext& app, lv_obj_t* parent) override {
        auto files = std::static_pointer_cast<Files>(app.getData());
        files->onShow(parent);
    }

    void onStart(AppContext& app) override {
        auto files = std::make_shared<Files>();
        app.setData(files);
    }

    void onResult(AppContext& app, Result result, const Bundle& bundle) override {
        auto files = std::static_pointer_cast<Files>(app.getData());
        files->onResult(result, bundle);
    }
};

extern const AppManifest manifest = {
    .id = "Files",
    .name = "Files",
    .icon = TT_ASSETS_APP_ICON_FILES,
    .type = Type::Hidden,
    .createApp = create<FilesApp>
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
