#include "app/files/View.h"
#include "app/files/State.h"

#include "app/AppContext.h"
#include "Assets.h"
#include "service/loader/Loader.h"

#include <memory>

namespace tt::app::files {

#define TAG "files_app"

extern const AppManifest manifest;

class FilesApp : public App {
    std::unique_ptr<View> view;
    std::shared_ptr<State> state;

public:
    FilesApp() {
        state = std::make_shared<State>();
        view = std::make_unique<View>(state);
    }

    void onShow(AppContext& appContext, lv_obj_t* parent) override {
        view->init(parent);
    }

    void onResult(AppContext& appContext, Result result, std::unique_ptr<Bundle> bundle) override {
        view->onResult(result, std::move(bundle));
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
