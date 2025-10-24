#include <esp_netif_sntp.h>
#include <Tactility/app/AppRegistration.h>
#include <Tactility/service/loader/Loader.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/service/wifi/Wifi.h>
#include <Tactility/Paths.h>
#include <Tactility/file/File.h>
#include <Tactility/network/Http.h>

#include <lvgl.h>
#include <esp_sntp.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/lvgl/Spinner.h>
#include <mbedtls/include/mbedtls/ssl_ciphersuites.h>

namespace tt::app::apphub {

constexpr auto* TAG = "AppHub";

extern const AppManifest manifest;

static std::string getVersionWithoutPostfix() {
    std::string version(TT_VERSION);
    auto index = version.find_first_of('-');
    if (index == std::string::npos) {
        return version;
    } else {
        return version.substr(0, index);
    }
}

static std::string getAppsJsonUrl() {
    return std::format("https://cdn.tactility.one/apps/{}/apps.json", getVersionWithoutPostfix());
}

class AppHubApp final : public App {

    lv_obj_t* contentWrapper = nullptr;
    std::string cachedAppsJsonFile = std::format("{}/apps.json", getTempPath());
    std::unique_ptr<Thread> thread;

    static std::shared_ptr<AppHubApp> _Nullable findAppInstance() {
        auto app_context = getCurrentAppContext();
        if (app_context->getManifest().appId != manifest.appId) {
            return nullptr;
        }
        return std::static_pointer_cast<AppHubApp>(app_context->getApp());
    }

    enum class State {
        Refreshing,
        ErrorTimeSync,
        ErrorConnection,
        ShowApps
    };

    static void onAppPressed(lv_event_t* e) {
        auto* self = static_cast<AppHubApp*>(lv_event_get_user_data(e));
    }

    static void onRefreshPressed(lv_event_t* e) {
        auto* self = static_cast<AppHubApp*>(lv_event_get_user_data(e));
        self->refresh();
    }

    void createRefreshButton() {
        auto* button = lv_button_create(contentWrapper);
        lv_obj_add_event_cb(button, onRefreshPressed, LV_EVENT_SHORT_CLICKED,this);
        auto* label = lv_label_create(button);
        lv_label_set_text(label, LV_SYMBOL_REFRESH);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }

    void onRefreshSuccess() {
        TT_LOG_I(TAG, "Request OK");
        auto lock = lvgl::getSyncLock()->asScopedLock();
        lock.lock();

        lv_obj_clean(contentWrapper);
        auto* label = lv_label_create(contentWrapper);
        lv_label_set_text(label, "Success!");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }

    void onRefreshError() {
        TT_LOG_E(TAG, "Request error");
        auto lock = lvgl::getSyncLock()->asScopedLock();
        lock.lock();

        showRefreshFailedError("Cannot reach server");
    }

    static void createAppWidget(const std::shared_ptr<AppManifest>& manifest, lv_obj_t* list) {
        lv_obj_t* btn = lv_list_add_button(list, nullptr, manifest->appName.c_str());
        lv_obj_add_event_cb(btn, &onAppPressed, LV_EVENT_SHORT_CLICKED, manifest.get());
    }

    void showRefreshFailedError(const char* message) {
        lv_obj_clean(contentWrapper);

        auto* label = lv_label_create(contentWrapper);
        lv_label_set_text(label, message);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        createRefreshButton();
    }

    void showNoInternet() {
        showRefreshFailedError("No Internet Connection");
    }

    void showTimeNotSynced() {
        showRefreshFailedError("Time is not synced yet.\nIt's required to establish a secure connection.");
    }

    void showApps() {
        lv_obj_clean(contentWrapper);
    }

    void refresh() {
        lv_obj_clean(contentWrapper);
        auto* spinner = lvgl::spinner_create(contentWrapper);
        lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);

        if (service::wifi::getRadioState() != service::wifi::RadioState::ConnectionActive) {
            showNoInternet();
            return;
        }

        if (file::isFile(cachedAppsJsonFile)) {
            showApps();
        }

        auto download_path = std::format("{}/app_hub.json", getTempPath());
        network::http::download(
            getAppsJsonUrl(),
            "/system/certificates/WE1.pem",
            download_path,
            [] {
                auto app = findAppInstance();
                if (app != nullptr) {
                    app->onRefreshSuccess();
                }
            },
            [] {
                auto app = findAppInstance();
                if (app != nullptr) {
                    app->onRefreshError();
                }
            }
        );
    }

public:

    void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) override {
        auto* toolbar = lvgl::toolbar_create(parent, app);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        contentWrapper = lv_obj_create(parent);
        lv_obj_set_width(contentWrapper, LV_PCT(100));
        lv_obj_align_to(contentWrapper, toolbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

        auto toolbar_height = lv_obj_get_height(toolbar);
        auto parent_content_height = lv_obj_get_content_height(parent);
        lv_obj_set_height(contentWrapper, parent_content_height - toolbar_height);

        refresh();
    }
};

extern const AppManifest manifest = {
    .appId = "AppHub",
    .appName = "App Hub",
    .appCategory = Category::System,
    .createApp = create<AppHubApp>,
};

} // namespace
