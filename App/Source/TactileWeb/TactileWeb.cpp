#define LV_USE_PRIVATE_API 1

#include <Tactility/app/App.h>
#include <Tactility/app/AppContext.h>
#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/Preferences.h>
#include <Tactility/lvgl/Keyboard.h>
#include <lvgl.h>
#include <string>	
#include <cstring>

#ifdef ESP_PLATFORM
#include <Tactility/service/wifi/Wifi.h>
#include <esp_http_client.h>
#include "html2text/html2text.h"
#endif

class TactileWeb : public tt::app::App {
private:
    struct AppWrapper {
        TactileWeb* app;
        AppWrapper(TactileWeb* app) : app(app) {}
    };

    lv_obj_t* toolbar = nullptr;
    lv_obj_t* url_input = nullptr;
    lv_obj_t* text_area = nullptr;
    lv_obj_t* wifi_button = nullptr;
    lv_obj_t* wifi_label = nullptr;
    lv_obj_t* loading_label = nullptr;
    lv_obj_t* retry_button = nullptr;
    tt::app::AppContext* context = nullptr;
    std::string last_url;

    static void url_input_cb(lv_event_t* e) {
        TactileWeb* app = static_cast<TactileWeb*>(lv_event_get_user_data(e));
        const char* url = lv_textarea_get_text(static_cast<const lv_obj_t*>(lv_event_get_target(e)));
        app->fetchAndDisplay(url);
        tt::lvgl::software_keyboard_hide();
    }

    static void wifi_connect_cb(lv_event_t* e) {
        tt::app::start("WifiManage");
    }

    static void retry_cb(lv_event_t* e) {
        const char* url = static_cast<char*>(lv_event_get_user_data(e));
        TactileWeb* app = static_cast<TactileWeb*>(lv_obj_get_user_data(static_cast<lv_obj_t*>(lv_event_get_target(e))));
        app->fetchAndDisplay(url);
    }

    void loadLastUrl() {
        tt::Preferences prefs("tactileweb");
        last_url = "http://example.com";
        prefs.optString("last_url", last_url);
        lv_textarea_set_text(url_input, last_url.c_str());
    }

    void saveLastUrl(const char* url) {
        last_url = url;
        tt::Preferences prefs("tactileweb");
        prefs.putString("last_url", last_url);
    }

    void showWifiPrompt() {
        lv_textarea_set_text(text_area, "No Wi-Fi connection available.");
        clearLoading();

        wifi_label = lv_label_create(text_area);
        lv_label_set_text(wifi_label, "Please connect to Wi-Fi to browse.");
        lv_obj_align(wifi_label, LV_ALIGN_TOP_MID, 0, 10);

        wifi_button = lv_btn_create(text_area);
        lv_obj_t* btn_label = lv_label_create(wifi_button);
        lv_label_set_text(btn_label, "Connect to Wi-Fi");
        lv_obj_center(btn_label);
        lv_obj_align(wifi_button, LV_ALIGN_CENTER, 0, 20);
        lv_obj_add_event_cb(wifi_button, wifi_connect_cb, LV_EVENT_CLICKED, this);
    }

    void showLoading() {
        clearLoading();
        loading_label = lv_label_create(text_area);
        lv_label_set_text(loading_label, "Loading...");
        lv_obj_center(loading_label);
    }

    void clearLoading() {
        if (loading_label) {
            lv_obj_del(loading_label);
            loading_label = nullptr;
        }
        if (retry_button) {
            lv_obj_del(retry_button);
            retry_button = nullptr;
        }
        if (wifi_label) {
            lv_obj_del(wifi_label);
            wifi_label = nullptr;
        }
        if (wifi_button) {
            lv_obj_del(wifi_button);
            wifi_button = nullptr;
        }
    }

    bool isValidUrl(const char* url) {
        return (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0);
    }

#ifdef ESP_PLATFORM
    void fetchAndDisplay(const char* url) {
        if (tt::service::wifi::getRadioState() != tt::service::wifi::RadioState::ConnectionActive) {
            showWifiPrompt();
            return;
        }

        if (!isValidUrl(url)) {
            lv_textarea_set_text(text_area, "Error: Invalid URL format.");
            return;
        }

        showLoading();
        lv_textarea_set_text(text_area, "");

        esp_http_client_config_t config = {
            .url = url,
            .method = HTTP_METHOD_GET,
            .timeout_ms = 5000,
            .skip_cert_common_name_check = true,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);

        char buffer[2048] = {0};
        esp_err_t err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            clearLoading();
            lv_textarea_set_text(text_area, "Error: Failed to connect to server.");
            TT_LOG_E("TactileWeb", "HTTP open failed: %s", esp_err_to_name(err));
            showRetryButton(url);
            esp_http_client_cleanup(client);
            return;
        }

        int content_length = esp_http_client_fetch_headers(client);
        if (content_length <= 0) {
            clearLoading();
            lv_textarea_set_text(text_area, "Error: No content received.");
            TT_LOG_E("TactileWeb", "No content length");
            showRetryButton(url);
            esp_http_client_cleanup(client);
            return;
        }

        int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
        if (len > 0) {
            buffer[len] = '\0';
            std::string html_content(buffer);
            std::string plain_text = html2text(html_content);
            clearLoading();
            lv_textarea_set_text(text_area, plain_text.c_str());
            saveLastUrl(url);
            TT_LOG_I("TactileWeb", "Displayed content from %s", url);
        } else {
            clearLoading();
            lv_textarea_set_text(text_area, "Error: Failed to read content.");
            TT_LOG_E("TactileWeb", "Read failed: %d", len);
            showRetryButton(url);
        }

        esp_http_client_cleanup(client);
    }
#else
    void fetchAndDisplay(const char* url) {
        // Simulator: display a static message
        clearLoading();
        lv_textarea_set_text(text_area, "Web browsing not supported in simulator");
    }
#endif

    void showRetryButton(const char* url) {
        retry_button = lv_btn_create(text_area);
        lv_obj_t* btn_label = lv_label_create(retry_button);
        lv_label_set_text(btn_label, "Retry");
        lv_obj_center(btn_label);
        lv_obj_align(retry_button, LV_ALIGN_CENTER, 0, 20);
        lv_obj_set_user_data(retry_button, this);
        lv_obj_add_event_cb(retry_button, retry_cb, LV_EVENT_CLICKED, const_cast<char*>(url));
    }

public:
    void onShow(tt::app::AppContext& app_context, lv_obj_t* parent) override {
        context = &app_context;

        toolbar = tt::lvgl::toolbar_create(parent, app_context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        url_input = lv_textarea_create(parent);
        lv_obj_set_size(url_input, LV_HOR_RES - 40, 30);
        lv_obj_align_to(url_input, toolbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_textarea_set_placeholder_text(url_input, "Enter URL (e.g., http://example.com)");
        lv_obj_add_event_cb(url_input, url_input_cb, LV_EVENT_READY, this);
        tt::lvgl::keyboard_add_textarea(url_input);

        text_area = lv_textarea_create(parent);
        lv_obj_set_size(text_area, LV_HOR_RES - 20, LV_VER_RES - 80);
        lv_obj_align_to(text_area, url_input, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

        loadLastUrl();
#ifdef ESP_PLATFORM
        if (tt::service::wifi::getRadioState() != tt::service::wifi::RadioState::ConnectionActive) {
            showWifiPrompt();
        } else {
            fetchAndDisplay(last_url.c_str());
        }
#else
        fetchAndDisplay(last_url.c_str());  // Simulator: show static message
#endif
    }

    void onHide(tt::app::AppContext& /* app_context */) override {
        // No additional cleanup needed; widgets are auto-destroyed
    }
};

extern const tt::app::AppManifest tactile_web_app = {
    .id = "TactileWeb",
    .name = "Tactile Web",
    .createApp = tt::app::create<TactileWeb>
};
