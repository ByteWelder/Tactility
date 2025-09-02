#define LV_USE_PRIVATE_API 1

#include <Tactility/app/App.h>
#include <Tactility/app/AppContext.h>
#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/Preferences.h>
#include <Tactility/lvgl/Keyboard.h>
#include <Tactility/PubSub.h>
#include <lvgl.h>
#include <string>
#include <cstring>
#include <memory>

#ifdef ESP_PLATFORM
#include <Tactility/service/wifi/Wifi.h>
#include <esp_http_client.h>
#include "html2text/html2text.h"
#endif



class TactileWeb : public tt::app::App {
private:
    // UI Components
    lv_obj_t* toolbar = nullptr;
    lv_obj_t* url_input = nullptr;
    lv_obj_t* text_area = nullptr;
    lv_obj_t* text_container = nullptr;
    lv_obj_t* wifi_button = nullptr;
    lv_obj_t* wifi_label = nullptr;
    lv_obj_t* loading_label = nullptr;
    lv_obj_t* retry_button = nullptr;
    lv_obj_t* status_label = nullptr;
    
    // App state
    tt::app::AppContext* context = nullptr;
    std::string last_url;
    std::string initial_url;
    bool is_loading = false;

#ifdef ESP_PLATFORM
    // WiFi subscription management with proper typing for new PubSub template
    std::shared_ptr<tt::PubSub<tt::service::wifi::WifiEvent>> wifi_pubsub;
    tt::PubSub<tt::service::wifi::WifiEvent>::SubscriptionHandle wifi_subscription = nullptr;

    // WiFi event callback with proper signature
    void wifi_event_callback(tt::service::wifi::WifiEvent event) {
        switch (event) {
            case tt::service::wifi::WifiEvent::ConnectionSuccess:
                updateStatusLabel("WiFi Connected", LV_PALETTE_GREEN);
                if (!last_url.empty() && isValidUrl(last_url.c_str())) {
                    fetchAndDisplay(last_url.c_str());
                }
                break;
            case tt::service::wifi::WifiEvent::Disconnected:
                updateStatusLabel("WiFi Disconnected", LV_PALETTE_RED);
                showWifiPrompt();
                break;
            case tt::service::wifi::WifiEvent::ConnectionPending:
                updateStatusLabel("WiFi Connecting...", LV_PALETTE_YELLOW);
                break;
            default:
                break;
        }
    }

    bool is_wifi_connected() const {
        return tt::service::wifi::getRadioState() == tt::service::wifi::RadioState::ConnectionActive;
    }
#endif

    // UI Event Handlers
    static void url_input_cb(lv_event_t* e) {
        auto* app = static_cast<TactileWeb*>(lv_event_get_user_data(e));
        const char* url = lv_textarea_get_text(static_cast<const lv_obj_t*>(lv_event_get_target(e)));
        
        if (app && url && strlen(url) > 0) {
            app->fetchAndDisplay(url);
            tt::lvgl::software_keyboard_hide();
        }
    }

    static void wifi_connect_cb(lv_event_t* e) {
        tt::app::start("WifiManage");
    }

    static void retry_cb(lv_event_t* e) {
        auto* app = static_cast<TactileWeb*>(lv_event_get_user_data(e));
        if (app && !app->last_url.empty()) {
            app->fetchAndDisplay(app->last_url.c_str());
        }
    }

    static void focus_url_cb(lv_event_t* e) {
        auto* app = static_cast<TactileWeb*>(lv_event_get_user_data(e));
        if (app && app->url_input) {
            lv_obj_add_state(app->url_input, LV_STATE_FOCUSED);
            lv_obj_scroll_to_view(app->url_input, LV_ANIM_ON);
        }
    }

    static void clear_cb(lv_event_t* e) {
        auto* app = static_cast<TactileWeb*>(lv_event_get_user_data(e));
        if (app && app->text_area) {
            lv_textarea_set_text(app->text_area, "");
        }
    }

    // URL and content management
    void loadLastUrl() {
        tt::Preferences prefs("tactileweb");
        initial_url = "http://example.com";
        prefs.optString("last_url", initial_url);
        last_url = initial_url;
    }

    void saveLastUrl(const char* url) {
        if (url && strlen(url) > 0) {
            last_url = url;
            tt::Preferences prefs("tactileweb");
            prefs.putString("last_url", last_url);
        }
    }

    bool isValidUrl(const char* url) const {
        if (!url || strlen(url) < 7) return false;
        return (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0);
    }

    // UI State Management
    void updateStatusLabel(const char* text, lv_palette_t color = LV_PALETTE_NONE) {
        if (!status_label && toolbar) {
            status_label = lv_label_create(toolbar);
            lv_obj_align(status_label, LV_ALIGN_LEFT_MID, 10, 0);
        }
        
        if (status_label) {
            lv_label_set_text(status_label, text);
            if (color != LV_PALETTE_NONE) {
                lv_obj_set_style_text_color(status_label, lv_palette_main(color), 0);
            }
        }
    }

    void showWifiPrompt() {
        clearContent();
        clearLoading();

        lv_textarea_set_text(text_area, "");

        wifi_label = lv_label_create(text_area);
        lv_label_set_text(wifi_label, "No Wi-Fi connection available.\nPlease connect to Wi-Fi to browse the web.");
        lv_obj_align(wifi_label, LV_ALIGN_TOP_MID, 0, 20);
        lv_obj_set_style_text_align(wifi_label, LV_TEXT_ALIGN_CENTER, 0);

        wifi_button = lv_btn_create(text_area);
        lv_obj_set_size(wifi_button, 150, 40);
        lv_obj_t* btn_label = lv_label_create(wifi_button);
        lv_label_set_text(btn_label, "Connect to Wi-Fi");
        lv_obj_center(btn_label);
        lv_obj_align(wifi_button, LV_ALIGN_CENTER, 0, 20);
        lv_obj_add_event_cb(wifi_button, wifi_connect_cb, LV_EVENT_CLICKED, this);

        updateStatusLabel("No WiFi Connection", LV_PALETTE_RED);
    }

    void showLoading(const char* url = nullptr) {
        if (is_loading) return;
        
        is_loading = true;
        clearContent();
        
        loading_label = lv_label_create(text_area);
        if (url) {
            std::string loading_text = "Loading: ";
            loading_text += url;
            lv_label_set_text(loading_label, loading_text.c_str());
        } else {
            lv_label_set_text(loading_label, "Loading...");
        }
        lv_obj_center(loading_label);
        lv_obj_set_style_text_align(loading_label, LV_TEXT_ALIGN_CENTER, 0);
        
        updateStatusLabel("Loading...", LV_PALETTE_YELLOW);
    }

    void clearLoading() {
        is_loading = false;
        if (loading_label) {
            lv_obj_del(loading_label);
            loading_label = nullptr;
        }
    }

    void clearContent() {
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

    void showError(const char* error_msg, const char* url = nullptr) {
        clearLoading();
        clearContent();
        
        std::string full_error = "Error: ";
        full_error += error_msg;
        
        lv_textarea_set_text(text_area, full_error.c_str());
        
        if (url && strlen(url) > 0) {
            showRetryButton();
        }
        
        updateStatusLabel("Error", LV_PALETTE_RED);
    }

    void showRetryButton() {
        if (!retry_button && text_area) {
            retry_button = lv_btn_create(text_area);
            lv_obj_set_size(retry_button, 100, 35);
            lv_obj_t* btn_label = lv_label_create(retry_button);
            lv_label_set_text(btn_label, "Retry");
            lv_obj_center(btn_label);
            lv_obj_align(retry_button, LV_ALIGN_BOTTOM_MID, 0, -20);
            lv_obj_add_event_cb(retry_button, retry_cb, LV_EVENT_CLICKED, this);
        }
    }

#ifdef ESP_PLATFORM
    void fetchAndDisplay(const char* url) {
        if (!url || strlen(url) == 0) {
            showError("Invalid URL provided");
            return;
        }

        if (!is_wifi_connected()) {
            showWifiPrompt();
            return;
        }

        if (!isValidUrl(url)) {
            showError("Invalid URL format. Please use http:// or https://");
            return;
        }

        showLoading(url);
        lv_textarea_set_text(text_area, "");

        // Configure HTTP client with better settings
        esp_http_client_config_t config = {};
        config.url = url;
        config.method = HTTP_METHOD_GET;
        config.timeout_ms = 10000;  // Increased timeout
        config.skip_cert_common_name_check = true;
        config.buffer_size = 4096;  // Larger buffer
        config.buffer_size_tx = 1024;
        
        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (!client) {
            showError("Failed to initialize HTTP client", url);
            return;
        }

        esp_err_t err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            showError("Failed to connect to server", url);
            TT_LOG_E("TactileWeb", "HTTP open failed: %s", esp_err_to_name(err));
            esp_http_client_cleanup(client);
            return;
        }

        // Get response code
        int status_code = esp_http_client_get_status_code(client);
        if (status_code < 200 || status_code >= 300) {
            std::string error_msg = "HTTP Error: " + std::to_string(status_code);
            showError(error_msg.c_str(), url);
            esp_http_client_cleanup(client);
            return;
        }

        int content_length = esp_http_client_fetch_headers(client);
        TT_LOG_I("TactileWeb", "Content length: %d, Status: %d", content_length, status_code);

        // Read content in chunks
        std::string html_content;
        char buffer[2048];
        int total_read = 0;
        const int max_content_size = 32768; // 32KB limit

        while (total_read < max_content_size) {
            int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
            if (len <= 0) break;
            
            buffer[len] = '\0';
            html_content.append(buffer, len);
            total_read += len;
            
            // Update loading progress
            if (loading_label) {
                std::string progress = "Loading... (" + std::to_string(total_read) + " bytes)";
                lv_label_set_text(loading_label, progress.c_str());
            }
        }

        esp_http_client_cleanup(client);

        if (html_content.empty()) {
            showError("No content received from server", url);
            return;
        }

        // Convert HTML to plain text - removed try-catch for -fno-exceptions compatibility
        std::string plain_text;
        // Attempt HTML to text conversion, fall back to raw HTML if it fails
        plain_text = html2text(html_content);
        if (plain_text.empty()) {
            plain_text = html_content; // Fallback to raw HTML
        }

        if (plain_text.empty()) {
            plain_text = "Content received but could not be processed.";
        }

        // Limit text length for display
        if (plain_text.length() > 8192) {
            plain_text = plain_text.substr(0, 8192) + "\n\n[Content truncated...]";
        }

        clearLoading();
        clearContent();
        lv_textarea_set_text(text_area, plain_text.c_str());
        
        // Scroll to top
        lv_obj_scroll_to_y(text_area, 0, LV_ANIM_ON);
        
        saveLastUrl(url);
        updateStatusLabel("Content Loaded", LV_PALETTE_GREEN);
        
        TT_LOG_I("TactileWeb", "Successfully loaded content from %s (%d bytes)", 
                 url, static_cast<int>(plain_text.length()));
    }
#else
    void fetchAndDisplay(const char* url) {
        clearLoading();
        clearContent();
        lv_textarea_set_text(text_area, "Web browsing is not supported in the simulator.\n\nThis feature requires ESP32 platform with WiFi connectivity.");
        updateStatusLabel("Simulator Mode", LV_PALETTE_GREY);
    }
#endif

public:
    void onShow(tt::app::AppContext& app_context, lv_obj_t* parent) override {
        context = &app_context;

        // Create toolbar with additional buttons
        toolbar = tt::lvgl::toolbar_create(parent, app_context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_scroll_dir(toolbar, LV_DIR_NONE);

        // URL focus button
        lv_obj_t* focus_btn = lv_btn_create(toolbar);
        lv_obj_set_size(focus_btn, 80, 30);
        lv_obj_t* focus_label = lv_label_create(focus_btn);
        lv_label_set_text(focus_label, "URL");
        lv_obj_center(focus_label);
        lv_obj_align(focus_btn, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_add_event_cb(focus_btn, focus_url_cb, LV_EVENT_CLICKED, this);

        // Clear button
        lv_obj_t* clear_btn = lv_btn_create(toolbar);
        lv_obj_set_size(clear_btn, 60, 30);
        lv_obj_t* clear_label = lv_label_create(clear_btn);
        lv_label_set_text(clear_label, "Clear");
        lv_obj_center(clear_label);
        lv_obj_align_to(clear_btn, focus_btn, LV_ALIGN_OUT_LEFT_MID, -5, 0);
        lv_obj_add_event_cb(clear_btn, clear_cb, LV_EVENT_CLICKED, this);

        // URL input field
        url_input = lv_textarea_create(parent);
        lv_obj_set_size(url_input, LV_HOR_RES - 40, 35);
        lv_obj_align_to(url_input, toolbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_textarea_set_placeholder_text(url_input, "Enter URL (e.g., http://example.com)");
        lv_textarea_set_one_line(url_input, true);
        lv_obj_add_event_cb(url_input, url_input_cb, LV_EVENT_READY, this);
        lv_obj_set_scrollbar_mode(url_input, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_scroll_dir(url_input, LV_DIR_NONE);

        // Content container with better styling
        text_container = lv_obj_create(parent);
        lv_obj_set_size(text_container, LV_HOR_RES - 20, LV_VER_RES - 90);
        lv_obj_align_to(text_container, url_input, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_obj_set_scrollbar_mode(text_container, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_style_border_width(text_container, 1, 0);
        lv_obj_set_style_border_color(text_container, lv_palette_main(LV_PALETTE_GREY), 0);

        // Text area for content display
        text_area = lv_textarea_create(text_container);
        lv_obj_set_size(text_area, lv_pct(100), lv_pct(100));
        lv_obj_set_pos(text_area, 0, 0);
        lv_obj_set_scrollbar_mode(text_area, LV_SCROLLBAR_MODE_AUTO);
        lv_textarea_set_text(text_area, "Enter a URL above to browse the web.");
        
        // Load saved settings
        loadLastUrl();
        lv_textarea_set_text(url_input, initial_url.c_str());

#ifdef ESP_PLATFORM
        // Initialize WiFi subscription with proper typing for new PubSub template
        wifi_pubsub = tt::service::wifi::getPubsub();
        if (wifi_pubsub) {
            wifi_subscription = wifi_pubsub->subscribe([this](tt::service::wifi::WifiEvent event) {
                wifi_event_callback(event);
            });
        }

        // Initial state check
        if (!is_wifi_connected()) {
            showWifiPrompt();
        } else {
            updateStatusLabel("WiFi Connected", LV_PALETTE_GREEN);
            if (!last_url.empty() && last_url != initial_url) {
                // Auto-load last URL if it's different from default
                fetchAndDisplay(last_url.c_str());
            }
        }
#else
        updateStatusLabel("Simulator Mode", LV_PALETTE_GREY);
#endif
    }

    void onHide(tt::app::AppContext& /* app_context */) override {
#ifdef ESP_PLATFORM
        // Properly cleanup WiFi subscription
        if (wifi_subscription && wifi_pubsub) {
            wifi_pubsub->unsubscribe(wifi_subscription);
            wifi_subscription = nullptr;
        }
        wifi_pubsub.reset();
#endif
        
        // Reset state
        is_loading = false;
        context = nullptr;
    }

    ~TactileWeb() override {
        // Ensure cleanup on destruction
#ifdef ESP_PLATFORM
        if (wifi_subscription && wifi_pubsub) {
            wifi_pubsub->unsubscribe(wifi_subscription);
        }
#endif
    }
};

extern const tt::app::AppManifest tactile_web_app = {
    .id = "TactileWeb",
    .name = "TactileWeb",
    .createApp = tt::app::create<TactileWeb>
};
