#include <Tactility/app/meshchat/MeshChatCommon.h>

#include <app/start.h>

#include <lvgl/widgets/card.h>

#include <tactility/log.h>

namespace tt::app::meshchat {

constexpr auto* TAG = "MeshChat";

void startApp(const char* id, int argc, const char* const argv[]) {
    AppStartContext context;
    if (app_start_context_from_id(id, &context) != ERROR_NONE) {
        LOG_E(TAG, "Failed to find %s", id);
        return;
    }
    app_start_context_set_arguments_ext(&context, argc, argv);
    AppInstanceId instanceId = 0;
    if (app_start_with_context(&context, &instanceId) != ERROR_NONE) {
        LOG_E(TAG, "Failed to start %s", id);
    }
}

lv_obj_t* createContent(lv_obj_t* parent) {
    auto* content = lv_obj_create(parent);
    // The theme's container padding is the margin of the cards, read before it is overridden below
    lv_obj_set_user_data(content, reinterpret_cast<void*>(static_cast<intptr_t>(lv_obj_get_style_pad_left(content, LV_PART_MAIN))));
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(content, LV_DPX(4), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(content, LV_DPX(4), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(content, 0, LV_STATE_DEFAULT);
    return content;
}

lv_obj_t* createCard(lv_obj_t* parent) {
    auto* card = lvgl_card_create(parent);
    // A percentage width ignores the margin, so the width is the available width without the margins
    const auto margin = static_cast<int32_t>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(parent)));
    lv_obj_update_layout(parent);
    lv_obj_set_size(card, lv_obj_get_content_width(parent) - 2 * margin, LV_SIZE_CONTENT);
    lv_obj_set_style_margin_all(card, margin, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(card, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(card, 0, LV_STATE_DEFAULT);
    return card;
}

}
