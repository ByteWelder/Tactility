#ifdef ESP_PLATFORM

#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/service/espnow/EspNow.h>

#include "Tactility/service/gui/Gui.h"
#include <cstdio>
#include <cstring>
#include <lvgl.h>

namespace tt::app::chat {

class ChatApp : public App {

    lv_obj_t* msg_list = nullptr;
    lv_obj_t* input_field = nullptr;

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
        const char* msg = lv_textarea_get_text(self->input_field);

        if (self->msg_list && msg && strlen(msg) > 0) {
            lv_obj_t* msg_label = lv_label_create(self->msg_list);
            lv_label_set_text(msg_label, msg);
            lv_obj_set_width(msg_label, lv_pct(100));
            lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_set_style_pad_all(msg_label, 2, 0);
            lv_obj_scroll_to_y(self->msg_list, lv_obj_get_scroll_y(self->msg_list) + 20, LV_ANIM_ON);
            lv_textarea_set_text(self->input_field, "");

            // TODO
//            service::espnow::send();
        }
    }

public:

    void onCreate(AppContext& appContext) override {
        // TODO: Move this to a configuration screen/app
        static const uint8_t key[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        static auto config = service::espnow::EspNowConfig(
            (uint8_t*)key,
            service::espnow::Mode::Station,
            1,
            false,
            false
        );

        service::espnow::enable(config);
    }

    void onDestroy(AppContext& appContext) override {
        if (service::espnow::isEnabled()) {
            service::espnow::disable();
        }
    }

    void onShow(AppContext& context, lv_obj_t* parent) override {

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
        input_field = lv_textarea_create(input_panel);
        lv_obj_set_flex_grow(input_field, 1);
        lv_obj_set_height(input_field, LV_PCT(100));
        lv_textarea_set_placeholder_text(input_field, "Type a message...");
        lv_textarea_set_one_line(input_field, true);
        service::gui::keyboardAddTextArea(input_field);

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
    .createApp = create<ChatApp>
};

}

#endif
