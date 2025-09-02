#include <Tactility/app/AppManifest.h>  // For AppManifest and app namespace
#include <Tactility/app/App.h>          // For App and AppContext
#include <Tactility/PubSub.h>           // For PubSub
#include <Tactility/Preferences.h>      // For Preferences
#include <Tactility/lvgl/Toolbar.h>     // For toolbar_create
#include <Tactility/lvgl/LvglSync.h>    // For LVGL synchronization
#include <lvgl.h>                       // Core LVGL library
#include <string>                       // For std::string

#ifdef ESP_PLATFORM
#include <Tactility/service/wifi/Wifi.h> // For Wi-Fi service
#include <esp_http_client.h>            // ESP-IDF HTTP client
#include <esp_wifi.h>                   // ESP-IDF Wi-Fi API
#include <cJSON.h>                      // JSON parsing
#endif

using namespace tt::app;

class TactilityNews : public App {
private:
    lv_obj_t* toolbar;
    lv_obj_t* news_container;
    lv_obj_t* news_list;
    lv_obj_t* wifi_label;
    lv_obj_t* wifi_button;
    AppContext* context;

#ifdef ESP_PLATFORM
    std::shared_ptr<tt::PubSub<tt::service::wifi::WifiEvent>> wifi_pubsub;
    tt::PubSub<tt::service::wifi::WifiEvent>::SubscriptionHandle wifi_subscription = nullptr;

    // Callback for "Connect to Wi-Fi" button
    static void wifi_connect_cb(lv_event_t* e) {
        tt::app::start("WifiManage"); // Launch Wi-Fi management app
    }

    // Wi-Fi event callback for PubSub
    static void wifi_event_cb(tt::service::wifi::WifiEvent event, void* context) {
        auto* self = static_cast<TactilityNews*>(context);
        if (event == tt::service::wifi::WifiEvent::ConnectionSuccess) {
            self->fetch_and_display_news();
        } else if (event == tt::service::wifi::WifiEvent::Disconnected) {
            self->redraw_ui();
        }
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

        // Get MAC address using ESP-IDF API
        uint8_t mac[6];
        esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, mac);
        if (err != ESP_OK) {
            TT_LOG_E("TactilityNews", "Failed to get MAC address: %s", esp_err_to_name(err));
            redraw_ui_with_error("Failed to get MAC address");
            return;
        }
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        std::string deviceId = mac_str;

        // Construct the API URL
        std::string url = "https://script.google.com/macros/s/AKfycbzNv6bljLjuZJW4bd1Mo5IZaW5Ppo6heTA_ru5CJjL6gdQzpEIWz3_MH0ZMOnzx_4io/exec?deviceId=" + deviceId + "&category=general";
        
        esp_http_client_config_t config = {};
        config.url = url.c_str();
        config.method = HTTP_METHOD_GET;
        
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_err_t err_http = esp_http_client_perform(client);

        if (err_http == ESP_OK) {
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
                                    std::string token = deviceToken->valuestring;
                                    tt::Preferences prefs("tactility_news");
                                    prefs.putString("device_token", token.c_str());
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
            TT_LOG_E("TactilityNews", "HTTP request failed: %s", esp_err_to_name(err_http));
            redraw_ui_with_error("Network error");
        }
        esp_http_client_cleanup(client);
    }

    // Display news titles in an LVGL list
    void display_news(cJSON* news_array) {
        auto lock = tt::lvgl::getSyncLock()->asScopedLock();
        if (!lock.lock(tt::lvgl::defaultLockTime)) {
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
#else
    // Simulator: no-op for fetch_and_display_news
    void fetch_and_display_news() {
        // No action in simulator; handled in redraw_ui
    }
#endif

    // Callback for "Refresh" button
    static void refresh_cb(lv_event_t* e) {
        TactilityNews* app = static_cast<TactilityNews*>(lv_event_get_user_data(e));
        app->fetch_and_display_news();
    }

    // Display an error message
    void redraw_ui_with_error(const char* error_msg) {
#ifdef ESP_PLATFORM
        auto lock = tt::lvgl::getSyncLock()->asScopedLock();
        if (!lock.lock(tt::lvgl::defaultLockTime)) {
            TT_LOG_E("TactilityNews", "LVGL lock failed in redraw");
            return;
        }
#endif

        lv_obj_clean(news_container);
        lv_obj_t* error_label = lv_label_create(news_container);
        lv_label_set_text(error_label, error_msg);
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
    }

    // Redraw the UI based on Wi-Fi status (ESP) or simulator message
    void redraw_ui() {
#ifdef ESP_PLATFORM
        auto lock = tt::lvgl::getSyncLock()->asScopedLock();
        if (!lock.lock(tt::lvgl::defaultLockTime)) {
            TT_LOG_E("TactilityNews", "LVGL lock failed in redraw");
            return;
        }
#endif

        lv_obj_clean(news_container);
#ifdef ESP_PLATFORM
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
#else
        // Simulator: show static message
        lv_obj_t* sim_label = lv_label_create(news_container);
        lv_label_set_text(sim_label, "News not supported in simulator");
        lv_obj_align(sim_label, LV_ALIGN_CENTER, 0, 0);
#endif
    }

public:
    void onShow(AppContext& app_context, lv_obj_t* parent) override {
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

#ifdef ESP_PLATFORM
        // Subscribe to Wi-Fi events
        wifi_pubsub = tt::service::wifi::getPubsub();
        wifi_subscription = wifi_pubsub->subscribe([this](tt::service::wifi::WifiEvent event) {
            wifi_event_cb(event, this);
        });
#endif

        redraw_ui();
    }

    void onHide(AppContext& app_context) override {
#ifdef ESP_PLATFORM
        if (wifi_subscription) {
            wifi_pubsub->unsubscribe(wifi_subscription);
            wifi_subscription = nullptr;
        }
        wifi_pubsub.reset();
#endif
    }
};

extern const AppManifest tactility_news_app = {
    .id = "TactilityNews",
    .name = "Tactility News",
    .createApp = create<TactilityNews>
};
