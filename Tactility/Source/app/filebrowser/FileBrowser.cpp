#include "Tactility/app/filebrowser/View.h"
#include "Tactility/app/filebrowser/State.h"
#include "Tactility/app/AppContext.h"

#include <Tactility/Assets.h>
#include <Tactility/service/loader/Loader.h>

#include <memory>

namespace tt::app::filebrowser {

constexpr auto* TAG = "FileBrowser";

extern const AppManifest manifest;

class FileBrowser : public App {
    std::unique_ptr<View> view;
    std::shared_ptr<State> state;

public:
    FileBrowser() {
        state = std::make_shared<State>();
        view = std::make_unique<View>(state);
    }

    void onShow(AppContext& appContext, lv_obj_t* parent) override {
        view->init(parent);
    }

    void onResult(AppContext& appContext, TT_UNUSED LaunchId launchId, Result result, std::unique_ptr<Bundle> bundle) override {
        view->onResult(launchId, result, std::move(bundle));
    }
};

extern const AppManifest manifest = {
    .id = "Files",
    .name = "Files",
    .icon = TT_ASSETS_APP_ICON_FILES,
    .type = Type::Hidden,
    .createApp = create<FileBrowser>
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
