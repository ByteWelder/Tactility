#ifdef ESP_PLATFORM

#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <lvgl.h>
#include <cstdio>
#include <cstring>

// ESPnow specific imports
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/task.h"

namespace tt::app::chat {

class ChatApp : public App {


    void initialize_espnow() {
        // 2. Cooperative WiFi initialization
        esp_err_t ret;
        for (int attempt = 0; attempt < 3; attempt++) {
            // 2a. Reset WiFi state
            ret = esp_wifi_stop();
            if (ret != ESP_OK && ret != ESP_ERR_WIFI_NOT_INIT) {
                vTaskDelay(10);
                continue;
            }

            // 2b. Feed watchdog between operations
            vTaskDelay(5);

            // 2c. Initialize with minimal config
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            cfg.nvs_enable = false;
            cfg.wifi_task_core_id = 1;  // Isolate to core 1

            ret = esp_wifi_init(&cfg);
            if (ret != ESP_OK) {
                vTaskDelay(10);
                continue;
            }

            // 2d. Configure WiFi
            esp_wifi_set_storage(WIFI_STORAGE_RAM);
            esp_wifi_set_mode(WIFI_MODE_STA);

            // 2e. Start with periodic watchdog feeding
            for (int start_attempt = 0; start_attempt < 3; start_attempt++) {
                ret = esp_wifi_start();
                if (ret == ESP_OK) break;
                vTaskDelay(20 * (start_attempt + 1));
            }

            // 2f. Initialize ESP-NOW
            if (esp_now_init() == ESP_OK) {
                return;  // Success
            }

            // Cleanup if failed
            esp_wifi_stop();
            esp_wifi_deinit();
            vTaskDelay(50);
        }
    }

    lv_obj_t* msg_list = nullptr;

    static void send_quick_msg(lv_event_t* e) {
        ChatApp* self = static_cast<ChatApp*>(lv_event_get_user_data(e));
        lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
        const char* msg = lv_label_get_text(lv_obj_get_child(btn, 0));

        if (self->msg_list && msg) {
            lv_obj_t* msg_label = lv_label_create(self->msg_list);
            lv_label_set_text(msg_label, msg);
            lv_obj_set_width(msg_label, lv_pct(100));
            lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_set_style_pad_all(msg_label, 2, 0);
            lv_obj_scroll_to_y(self->msg_list, lv_obj_get_scroll_y(self->msg_list) + 20, LV_ANIM_ON);
        }
    }

    static void send_message(lv_event_t* e) {
        ChatApp* self = static_cast<ChatApp*>(lv_event_get_user_data(e));
        lv_obj_t* ta = static_cast<lv_obj_t*>(lv_event_get_target(e));
        const char* msg = lv_textarea_get_text(ta);

        if (self->msg_list && msg && strlen(msg) > 0) {
            lv_obj_t* msg_label = lv_label_create(self->msg_list);
            lv_label_set_text(msg_label, msg);
            lv_obj_set_width(msg_label, lv_pct(100));
            lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_set_style_pad_all(msg_label, 2, 0);
            lv_obj_scroll_to_y(self->msg_list, lv_obj_get_scroll_y(self->msg_list) + 20, LV_ANIM_ON);
            lv_textarea_set_text(ta, "");
        }
    }

public:
    void onShow(AppContext& context, lv_obj_t* parent) override {


        // initialize the ESPnow
        initialize_espnow();

        // Create toolbar
        lv_obj_t* toolbar = tt::lvgl::toolbar_create(parent, context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);
        const int toolbar_height = lv_obj_get_height(toolbar);

        // Message list
        msg_list = lv_list_create(parent);
        lv_obj_set_size(msg_list, lv_pct(75), lv_pct(43));
        lv_obj_align(msg_list, LV_ALIGN_TOP_LEFT, 5, toolbar_height + 45);
        lv_obj_set_style_bg_color(msg_list, lv_color_hex(0xEEEEEE), 0);
        lv_obj_set_style_border_width(msg_list, 1, 0);
        lv_obj_set_style_pad_all(msg_list, 5, 0);

        // Quick message panel
        lv_obj_t* quick_panel = lv_obj_create(parent);
        lv_obj_set_size(quick_panel, lv_pct(20), lv_pct(58));
        lv_obj_align(quick_panel, LV_ALIGN_TOP_RIGHT, -5, toolbar_height + 15);  // Adjusted to match
        lv_obj_set_flex_flow(quick_panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(quick_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(quick_panel, 5, 0);

        // Quick message buttons
        const char* quick_msgs[] = {":-)", "+1", ":-("};
        for (size_t i = 0; i < sizeof(quick_msgs)/sizeof(quick_msgs[0]); i++) {
            lv_obj_t* btn = lv_btn_create(quick_panel);
            lv_obj_set_size(btn, lv_pct(75), 25);
            lv_obj_add_event_cb(btn, send_quick_msg, LV_EVENT_CLICKED, this);

            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_text(label, quick_msgs[i]);
            lv_obj_center(label);
        }

        // Input panel
        lv_obj_t* input_panel = lv_obj_create(parent);
        lv_obj_set_size(input_panel, lv_pct(95), 60);
        lv_obj_align(input_panel, LV_ALIGN_BOTTOM_MID, 0, -5);
        lv_obj_set_flex_flow(input_panel, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(input_panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(input_panel, 5, 0);

        // Input field
        lv_obj_t* input_field = lv_textarea_create(input_panel);
        lv_obj_set_flex_grow(input_field, 1);
        lv_obj_set_height(input_field, LV_PCT(100));
        lv_textarea_set_placeholder_text(input_field, "Type a message...");
        lv_textarea_set_one_line(input_field, true);

        // Send button
        lv_obj_t* send_btn = lv_btn_create(input_panel);
        lv_obj_set_size(send_btn, 80, LV_PCT(100));
        lv_obj_add_event_cb(send_btn, send_message, LV_EVENT_CLICKED, this);

        lv_obj_t* btn_label = lv_label_create(send_btn);
        lv_label_set_text(btn_label, "Send");
        lv_obj_center(btn_label);
    }

    ~ChatApp() override = default;
};

extern const AppManifest manifest = {
    .id = "Chat",
    .name = "Chat",
    .createApp = []() -> std::shared_ptr<App> { return std::make_shared<ChatApp>(); }
};

}

#endif
