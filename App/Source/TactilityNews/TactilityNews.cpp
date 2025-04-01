#include <Tactility/app/AppManifest.h>
#include <Tactility/app/AppContext.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/Preferences.h>
#include <Tactility/service/wifi/Wifi.h>
#include <lvgl.h>
#include "esp_http_client.h"
#include <cJSON.h>
#include <WiFi.h>

namespace tt::app::tactility_news {

class TactilityNews : public tt::app::App {
private:
    lv_obj_t* toolbar;
    lv_obj_t* news_container;
    lv_obj_t* news_list;
    lv_obj_t* wifi_label;
    lv_obj_t* wifi_button;
    tt::AppContext* context;
    std::shared_ptr<tt::service::PubSub> wifi_pubsub;
    std::shared_ptr<tt::service::PubSub::Subscription> wifi_subscription;

    // Callback for "Connect to Wi-Fi" button
    static void wifi_connect_cb(lv_event_t* e) {
        tt::app::start("WifiManage"); // Launch Wi-Fi management app
    }

    // Callback for "Refresh" button
    static void refresh_cb(lv_event_t* e) {
        TactilityNews* app = static_cast<TactilityNews*>(lv_event_get_user_data(e));
        app->fetch_and_display_news();
    }

    // Check Wi-Fi connection status
    bool is_wifi_connected() {
        return tt::service::wifi::getRadioState() == tt::service::wifi::RadioState::ConnectionActive;
    }

    // Fetch news from the API and display it
    void fetch_and_display_news() {
        if (!is_wifi_connected()) {
            redraw_ui();
            return;
        }

        String deviceId = WiFi.macAddress();
        String url = "https://script.google.com/macros/s/[YOUR_GAS_ID]/exec?deviceId=" + deviceId + "&category=general";

        esp_http_client_config_t config = {
            .url = url.c_str(),
            .method = HTTP_METHOD_GET,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_err_t err = esp_http_client_perform(client);

        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            if (status_code == 200) {
                int content_length = esp_http_client_get_content_length(client);
                if (content_length > 0) {
                    char* buffer = (char*)malloc(content_length + 1);
                    if (buffer != nullptr) {
                        int read_len = esp_http_client_read(client, buffer, content_length);
                        if (read_len == content_length) {
                            buffer[content_length] = '\0';
                            cJSON* root = cJSON_Parse(buffer);
                            if (root != nullptr) {
                                // Store deviceToken
                                cJSON* deviceToken = cJSON_GetObjectItem(root, "deviceToken");
                                if (deviceToken != nullptr && cJSON_IsString(deviceToken)) {
                                    String token = deviceToken->valuestring;
                                    tt::Preferences prefs;
                                    prefs.begin("tactility_news");
                                    prefs.putString("device_token", token.c_str());
                                    prefs.end();
                                }
                                // Display news
                                cJSON* news = cJSON_GetObjectItem(root, "news");
                                if (news != nullptr && cJSON_IsArray(news)) {
                                    display_news(news);
                                } else {
                                    TT_LOG_E("TactilityNews", "No news array in response");
                                    redraw_ui_with_error("No news available");
                                }
                                cJSON_Delete(root);
                            } else {
                                TT_LOG_E("TactilityNews", "JSON parse error");
                                redraw_ui_with_error("Failed to parse news");
                            }
                        } else {
                            TT_LOG_E("TactilityNews", "Failed to read content");
                            redraw_ui_with_error("Failed to load news");
                        }
                        free(buffer);
                    } else {
                        TT_LOG_E("TactilityNews", "Memory allocation failed");
                        redraw_ui_with_error("Memory error");
                    }
                }
            } else {
                TT_LOG_E("TactilityNews", "HTTP status: %d", status_code);
                redraw_ui_with_error("Failed to load news");
            }
        } else {
            TT_LOG_E("TactilityNews", "HTTP request failed: %s", esp_err_to_name(err));
            redraw_ui_with_error("Network error");
        }
        esp_http_client_cleanup(client);
    }

    // Display news titles in an LVGL list
    void display_news(cJSON* news_array) {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        if (!lock.lock(lvgl::defaultLockTime)) {
            TT_LOG_E("TactilityNews", "LVGL lock failed");
            return;
        }

        lv_obj_clean(news_container);
        news_list = lv_list_create(news_container);
        lv_obj_set_size(news_list, LV_PCT(100), LV_PCT(100));

        cJSON* item = nullptr;
        cJSON_ArrayForEach(item, news_array) {
            cJSON* title = cJSON_GetObjectItem(item, "title");
            if (title != nullptr && cJSON_IsString(title)) {
                lv_list_add_btn(news_list, NULL, title->valuestring);
            }
        }
    }

    // Display an error message
    void redraw_ui_with_error(const char* error_msg) {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        if (!lock.lock(lvgl::defaultLockTime)) {
            TT_LOG_E("TactilityNews", "LVGL lock failed in redraw");
            return;
        }

        lv_obj_clean(news_container);
        lv_obj_t* error_label = lv_label_create(news_container);
        lv_label_set_text(error_label, error_msg);
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
    }

    // Redraw the UI based on Wi-Fi status
    void redraw_ui() {
        auto lock = lvgl::getSyncLock()->asScopedLock();
        if (!lock.lock(lvgl::defaultLockTime)) {
            TT_LOG_E("TactilityNews", "LVGL lock failed in redraw");
            return;
        }

        lv_obj_clean(news_container);
        if (!is_wifi_connected()) {
            wifi_label = lv_label_create(news_container);
            lv_label_set_text(wifi_label, "No Wi-Fi - News unavailable");
            lv_obj_align(wifi_label, LV_ALIGN_CENTER, 0, -20);

            wifi_button = lv_btn_create(news_container);
            lv_obj_t* btn_label = lv_label_create(wifi_button);
            lv_label_set_text(btn_label, "Connect to Wi-Fi");
            lv_obj_center(btn_label);
            lv_obj_align(wifi_button, LV_ALIGN_CENTER, 0, 20);
            lv_obj_add_event_cb(wifi_button, wifi_connect_cb, LV_EVENT_CLICKED, context);
        } else {
            fetch_and_display_news();
        }
    }

public:
    void onShow(tt::AppContext& app_context, lv_obj_t* parent) override {
        context = &app_context;

        // Create toolbar with refresh button
        toolbar = tt::lvgl::toolbar_create(parent, app_context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        lv_obj_t* refresh_btn = lv_btn_create(toolbar);
        lv_obj_t* refresh_label = lv_label_create(refresh_btn);
        lv_label_set_text(refresh_label, "Refresh");
        lv_obj_center(refresh_label);
        lv_obj_align(refresh_btn, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_add_event_cb(refresh_btn, refresh_cb, LV_EVENT_CLICKED, this);

        // Create news container
        news_container = lv_obj_create(parent);
        lv_obj_set_size(news_container, LV_PCT(100), LV_PCT(80));
        lv_obj_align_to(news_container, toolbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

        // Subscribe to Wi-Fi events
        wifi_pubsub = tt::service::wifi::getPubsub();
        wifi_subscription = wifi_pubsub->subscribe([this](const tt::service::Event* event) {
            if (event->type == tt::service::wifi::EventType::ConnectionSuccess) {
                fetch_and_display_news();
            } else if (event->type == tt::service::wifi::EventType::Disconnected) {
                redraw_ui();
            }
        });

        redraw_ui();
    }

    void onHide(tt::AppContext& app_context) override {
        wifi_subscription.reset();
        wifi_pubsub.reset();
    }
};

extern const tt::app::AppManifest tactility_news_app = {
    .id = "TactilityNews",
    .name = "TactilityNews",
    .createApp = tt::app::create<TactilityNews>
};

} // namespace tt::app::tactility_news
