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

static std::string getAppsJsonHttpRequest() {
    return std::format(
        "GET {} HTTP/1.1\r\n"
         "Host: cdn.tactility.one\r\n"
         "User-Agent: Tactility/" TT_VERSION " " CONFIG_IDF_TARGET "\r\n"
         "\r\n",
         getAppsJsonUrl()
    );
}

class AppHubApp final : public App {

    lv_obj_t* spinner = nullptr;
    lv_obj_t* refreshButton = nullptr;
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

    void onRefreshSuccess() {
        TT_LOG_I(TAG, "Request OK");
    }

    void onRefreshError() {
        TT_LOG_E(TAG, "Request error");
    }

    static void createAppWidget(const std::shared_ptr<AppManifest>& manifest, lv_obj_t* list) {
        lv_obj_t* btn = lv_list_add_button(list, nullptr, manifest->appName.c_str());
        lv_obj_add_event_cb(btn, &onAppPressed, LV_EVENT_SHORT_CLICKED, manifest.get());
    }

    void showNoInternet() {
        lv_obj_add_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clean(contentWrapper);
        auto* label = lv_label_create(contentWrapper);
        lv_label_set_text(label, "WiFi is not connected");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }

    void showTimeNotSynced() {
        lv_obj_add_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clean(contentWrapper);
        auto* label = lv_label_create(contentWrapper);
        lv_label_set_text(label, "Time is not synced yet.\nIt's required to establish a secure connection.");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }

    void showApps() {
        lv_obj_add_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clean(contentWrapper);
    }

    void refresh() {
        lv_obj_add_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);

        if (service::wifi::getRadioState() != service::wifi::RadioState::ConnectionActive) {
            showNoInternet();
            return;
        }

        if (file::isFile(cachedAppsJsonFile)) {
            showApps();
        }

        lv_obj_remove_flag(refreshButton, LV_OBJ_FLAG_HIDDEN);

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

        spinner = lvgl::toolbar_add_spinner_action(toolbar);
        refreshButton = lvgl::toolbar_add_image_button_action(toolbar, LV_SYMBOL_REFRESH, onRefreshPressed, this);

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
