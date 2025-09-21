#include <Tactility/app/files/View.h>
#include <Tactility/app/files/State.h>
#include <Tactility/app/AppContext.h>

#include <Tactility/Assets.h>
#include <Tactility/service/loader/Loader.h>

#include <memory>

namespace tt::app::files {

extern const AppManifest manifest;

class FilesApp final : public App {

    std::unique_ptr<View> view;
    std::shared_ptr<State> state;

public:

    FilesApp() {
        state = std::make_shared<State>();
        view = std::make_unique<View>(state);
    }

    void onShow(AppContext& appContext, lv_obj_t* parent) override {
        view->init(appContext, parent);
    }

    void onResult(AppContext& appContext, TT_UNUSED LaunchId launchId, Result result, std::unique_ptr<Bundle> bundle) override {
        view->onResult(launchId, result, std::move(bundle));
    }
};

extern const AppManifest manifest = {
    .appId = "Files",
    .appName = "Files",
    .appIcon = TT_ASSETS_APP_ICON_FILES,
    .appCategory = Category::System,
    .appFlags = AppManifest::Flags::Hidden,
    .createApp = create<FilesApp>
};

void start() {
    service::loader::startApp(manifest.appId);
}

} // namespace
