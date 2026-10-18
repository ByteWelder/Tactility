#include "app/location.h"
#ifdef ESP_PLATFORM
#include <esp_random.h>
#else
#include <random>
#endif

#include <Tactility/app/inputdialog/InputDialog.h>
#include <Tactility/app/meshsettings/MeshChannels.h>
#include <Tactility/app/selectiondialog/SelectionDialog.h>
#include <Tactility/settings/RadioSettings.h>

#include <app/event.h>
#include <app/manager.h>
#include <app/manifest.h>
#include <app/scheduler.h>
#include <app/stream.h>

#include <lvgl_window_manager/window_manager.h>

#include <mesh/mesh.h>

#include <tactility/check.h>
#include <tactility/concurrent/task_event_group.h>
#include <tactility/log.h>

#include <lvgl/fonts.h>
#include <lvgl/icons/shared.h>
#include <lvgl/lvgl.h>
#include <lvgl/widgets/card.h>
#include <lvgl/widgets/toolbar.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace tt::app::meshsettings {

extern const ::AppManifest manifest;

namespace {

constexpr auto* TAG = "MeshSettings";

struct Context {
    uint32_t appInstanceId;
    TaskEventGroup* eventGroup = nullptr;
    uint32_t selectServiceBit = 0;
    uint32_t selectRegionBit = 0;
    uint32_t selectPresetBit = 0;
    uint32_t addChannelBit = 0;
    uint32_t changeNameBit = 0;
    uint32_t selectRadioBit = 0;
    uint32_t serviceDialogId = 0;
    uint32_t regionDialogId = 0;
    uint32_t presetDialogId = 0;
    uint32_t channelDialogId = 0;
    uint32_t nameDialogId = 0;
    uint32_t radioDialogId = 0;
    AppStream channelStream {};
    char channelBuffer[MESH_NAME_MAX_LENGTH + 1] = {};
    AppStream nameStream {};
    char nameBuffer[MESH_NAME_MAX_LENGTH + 1] = {};
    // Shown by updateLabels(), which can run on another app's thread and so must not query the services.
    // The app's own thread fills these with loadState().
    std::string serviceText;
    std::string nameText;
    std::string radioText = "-";
    bool radioChoice = false;
    std::string regionText;
    std::string presetText;
    bool hasPresets = false;
    std::string statusText;
    /** Channel names of the active backend, empty when it has no channels */
    std::string channelsText;
    /** Index 0 is "Disabled", the rest are backend ids */
    std::vector<std::string> serviceIds;
    std::vector<std::string> presetNames;
    /** Device names of the radios, in the order of the selection dialog */
    std::vector<std::string> radioNames;
    // Valid while the window is shown
    lv_obj_t* serviceLabel = nullptr;
    lv_obj_t* nameLabel = nullptr;
    lv_obj_t* radioButton = nullptr;
    lv_obj_t* radioLabel = nullptr;
    lv_obj_t* regionLabel = nullptr;
    lv_obj_t* presetRow = nullptr;
    lv_obj_t* presetLabel = nullptr;
    lv_obj_t* channelsRow = nullptr;
    lv_obj_t* channelsLabel = nullptr;
    lv_obj_t* statusLabel = nullptr;
};

std::string getServiceLabel(const std::string& id) {
    if (id == "meshcore") return "MeshCore";
    if (id == "meshtastic") return "Meshtastic";
    return id;
}

std::string getCurrentService() {
    if (!mesh_is_enabled()) {
        return "Disabled";
    }
    char id[MESH_BACKEND_ID_MAX_LENGTH + 1];
    return (mesh_get_backend(id, sizeof(id)) == ERROR_NONE) ? getServiceLabel(id) : "Enabled";
}

std::string getStatus() {
    const auto state = mesh_get_state();
    if (state == MESH_STATE_NO_REGION) {
        return "Select a region to turn on the radio";
    }
    return mesh_state_to_string(state);
}

/** Requires the LVGL lock */
void updateLabels(Context* ctx) {
    if (ctx->serviceLabel == nullptr) {
        return;
    }
    lv_label_set_text(ctx->serviceLabel, ctx->serviceText.c_str());
    lv_label_set_text(ctx->nameLabel, ctx->nameText.c_str());
    // The radio is always shown, but it can only be changed when there are several
    if (ctx->radioChoice) {
        lv_obj_remove_state(ctx->radioButton, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(ctx->radioButton, LV_STATE_DISABLED);
    }
    lv_label_set_text(ctx->radioLabel, ctx->radioText.c_str());
    lv_label_set_text(ctx->regionLabel, ctx->regionText.c_str());
    lv_obj_set_hidden(ctx->presetRow, !ctx->hasPresets);
    lv_label_set_text(ctx->presetLabel, ctx->presetText.c_str());
    lv_obj_set_hidden(ctx->channelsRow, ctx->channelsText.empty());
    lv_label_set_text(ctx->channelsLabel, ctx->channelsText.c_str());
    lv_label_set_text(ctx->statusLabel, ctx->statusText.c_str());
}

/** Queries the service thread, so it's called without the LVGL lock */
void loadChannels(Context* ctx) {
    MeshChannel channels[8];
    size_t count = 0;
    ctx->channelsText.clear();
    if (mesh_get_channels(channels, 8, &count) != ERROR_NONE) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        if (!ctx->channelsText.empty()) {
            ctx->channelsText += ", ";
        }
        ctx->channelsText += channels[i].name;
    }
}

/** The device names of the radios, and the names to show for them: the kind of radio, plus the device name when there are several of a kind */
void loadRadios(std::vector<std::string>& names, std::vector<std::string>& labels) {
    names.clear();
    labels.clear();
    std::vector<MeshRadioKind> kinds;
    const size_t count = mesh_get_radio_count();
    for (size_t i = 0; i < count; i++) {
        char name[MESH_NAME_MAX_LENGTH + 1];
        MeshRadioKind kind;
        if (mesh_get_radio_name(i, name, sizeof(name)) == ERROR_NONE && mesh_get_radio_kind(i, &kind) == ERROR_NONE) {
            names.emplace_back(name);
            kinds.push_back(kind);
        }
    }
    for (size_t i = 0; i < names.size(); i++) {
        const char* kindText = (kinds[i] == MESH_RADIO_LORA) ? "LoRa" : "ESP-NOW";
        const auto sameKind = std::count(kinds.begin(), kinds.end(), kinds[i]);
        labels.push_back(sameKind > 1 ? std::string(kindText) + " " + names[i] : std::string(kindText));
    }
}

/** Reads everything that updateLabels() shows. Queries the service thread, so it's called without the LVGL lock. */
void loadState(Context* ctx) {
    loadChannels(ctx);
    ctx->serviceText = getCurrentService();
    char name[MESH_NAME_MAX_LENGTH + 1];
    mesh_get_node_name(name, sizeof(name));
    ctx->nameText = name;
    // A choice is only possible when there is more than one radio
    char radio[MESH_NAME_MAX_LENGTH + 1] = "-";
    mesh_get_radio(radio, sizeof(radio));
    std::vector<std::string> radioNames;
    std::vector<std::string> radioLabels;
    loadRadios(radioNames, radioLabels);
    ctx->radioText = radio;
    for (size_t i = 0; i < radioNames.size(); i++) {
        if (radioNames[i] == radio) {
            ctx->radioText = radioLabels[i];
        }
    }
    ctx->radioChoice = radioNames.size() >= 2;
    ctx->regionText = settings::radio::getRegionLabel(settings::radio::loadRegion());
    char preset[MESH_NAME_MAX_LENGTH + 1];
    ctx->hasPresets = mesh_is_enabled() && mesh_get_preset(preset, sizeof(preset)) == ERROR_NONE;
    ctx->presetText = ctx->hasPresets ? preset : "";
    ctx->statusText = getStatus();
}

void onAddChannelPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->addChannelBit);
}

void onSelectRadioPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->selectRadioBit);
}

void onChangeNamePressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->changeNameBit);
}

void startNameDialog(Context* ctx) {
    if (ctx->nameDialogId != 0) {
        return;
    }
    char name[MESH_NAME_MAX_LENGTH + 1];
    mesh_get_node_name(name, sizeof(name));
    ctx->nameDialogId = inputdialog::start(
        ctx->appInstanceId,
        "Name",
        "The name that other nodes see",
        name,
        ctx->nameStream,
        ctx->nameBuffer,
        sizeof(ctx->nameBuffer),
        ctx->eventGroup
    );
}

void onNameDialogResult(Context* ctx, int32_t result) {
    if (result == 0) {
        char name[sizeof(Context::nameBuffer)] = {};
        const size_t length = app_stream_read(&ctx->nameStream, name, sizeof(name) - 1);
        name[length] = '\0';
        const error_t error = mesh_set_node_name(name);
        if (error != ERROR_NONE) {
            LOG_E(TAG, "Failed to set the name: %s", error_to_string(error));
        }
    }
    app_stream_unsubscribe(&ctx->nameStream);
}

void startChannelDialog(Context* ctx) {
    if (ctx->channelDialogId != 0) {
        return;
    }
    ctx->channelDialogId = inputdialog::start(
        ctx->appInstanceId,
        "Join channel",
        "Hashtag channel name, e.g. #test",
        "#",
        ctx->channelStream,
        ctx->channelBuffer,
        sizeof(ctx->channelBuffer),
        ctx->eventGroup
    );
}

void onChannelDialogResult(Context* ctx, int32_t result) {
    if (result == 0) {
        char name[sizeof(Context::channelBuffer)] = {};
        const size_t length = app_stream_read(&ctx->channelStream, name, sizeof(name) - 1);
        name[length] = '\0';
        const error_t error = joinHashtagChannel(name);
        if (error != ERROR_NONE) {
            LOG_E(TAG, "Failed to join channel %s: %s", name, error_to_string(error));
        }
    }
    app_stream_unsubscribe(&ctx->channelStream);
}

void onBackPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    app_event_emit_close(ctx->appInstanceId);
}

void onSelectServicePressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->selectServiceBit);
}

void onSelectRegionPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->selectRegionBit);
}

void onSelectPresetPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->selectPresetBit);
}

void startServiceSelection(Context* ctx) {
    if (ctx->serviceDialogId != 0) {
        return;
    }
    ctx->serviceIds = { "" };
    std::vector<std::string> items = { "Disabled" };
    const size_t count = mesh_get_backend_count();
    for (size_t i = 0; i < count; i++) {
        char id[MESH_BACKEND_ID_MAX_LENGTH + 1];
        if (mesh_get_backend_id(i, id, sizeof(id)) == ERROR_NONE) {
            ctx->serviceIds.emplace_back(id);
            items.push_back(getServiceLabel(id));
        }
    }
    ctx->serviceDialogId = selectiondialog::start(ctx->appInstanceId, "Mesh network", items);
}

void onServiceSelected(Context* ctx, int32_t selected) {
    if (selected < 0 || selected >= static_cast<int32_t>(ctx->serviceIds.size())) {
        return;
    }
    if (selected == 0) {
        mesh_set_enabled(false);
    } else {
        mesh_set_backend(ctx->serviceIds[selected].c_str());
        mesh_set_enabled(true);
    }
}

void startRadioSelection(Context* ctx) {
    if (ctx->radioDialogId != 0) {
        return;
    }
    std::vector<std::string> labels;
    loadRadios(ctx->radioNames, labels);
    if (ctx->radioNames.size() > 1) {
        ctx->radioDialogId = selectiondialog::start(ctx->appInstanceId, "Radio", labels);
    }
}

void onRadioSelected(Context* ctx, int32_t selected) {
    if (selected >= 0 && selected < static_cast<int32_t>(ctx->radioNames.size())) {
        const error_t error = mesh_set_radio(ctx->radioNames[selected].c_str());
        if (error != ERROR_NONE) {
            LOG_E(TAG, "Failed to select the radio: %s", error_to_string(error));
        }
    }
}

void startRegionSelection(Context* ctx) {
    if (ctx->regionDialogId != 0) {
        return;
    }
    std::vector<std::string> items;
    for (int region = RADIO_REGION_UNSET + 1; region < RADIO_REGION_COUNT; region++) {
        items.emplace_back(settings::radio::getRegionLabel(static_cast<RadioRegion>(region)));
    }
    ctx->regionDialogId = selectiondialog::start(ctx->appInstanceId, "Region", items);
}

void onRegionSelected(int32_t selected) {
    if (selected >= 0 && selected < RADIO_REGION_COUNT - 1) {
        settings::radio::saveRegion(static_cast<RadioRegion>(selected + 1));
    }
}

void startPresetSelection(Context* ctx) {
    if (ctx->presetDialogId != 0) {
        return;
    }
    ctx->presetNames.clear();
    const size_t count = mesh_get_preset_count();
    for (size_t i = 0; i < count; i++) {
        char name[MESH_NAME_MAX_LENGTH + 1];
        if (mesh_get_preset_name(i, name, sizeof(name)) == ERROR_NONE) {
            ctx->presetNames.emplace_back(name);
        }
    }
    if (!ctx->presetNames.empty()) {
        ctx->presetDialogId = selectiondialog::start(ctx->appInstanceId, "Preset", ctx->presetNames);
    }
}

void onPresetSelected(Context* ctx, int32_t selected) {
    if (selected >= 0 && selected < static_cast<int32_t>(ctx->presetNames.size())) {
        mesh_set_preset(ctx->presetNames[selected].c_str());
    }
}

lv_obj_t* createRow(lv_obj_t* parent) {
    auto* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_width(row, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(row, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_scrollable(row, false);
    return row;
}

/** Creates a title with the setting's value below it. @return the value label */
lv_obj_t* createTitleAndValue(lv_obj_t* row, const char* title) {
    auto* texts = lv_obj_create(row);
    lv_obj_set_height(texts, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(texts, 1);
    lv_obj_set_flex_flow(texts, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(texts, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(texts, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(texts, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(texts, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_scrollable(texts, false);

    auto* titleLabel = lv_label_create(texts);
    lv_obj_set_width(titleLabel, LV_PCT(100));
    lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_label_set_text(titleLabel, title);

    auto* valueLabel = lv_label_create(texts);
    lv_obj_set_width(valueLabel, LV_PCT(100));
    lv_obj_set_style_text_font(valueLabel, lvgl_get_text_font(FONT_SIZE_SMALL), LV_STATE_DEFAULT);
    lv_label_set_long_mode(valueLabel, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    return valueLabel;
}

lv_obj_t* createChangeButton(lv_obj_t* row, lv_event_cb_t callback, Context* ctx) {
    auto* button = lv_button_create(row);
    auto* icon = lv_label_create(button);
    lv_obj_set_style_text_font(icon, lvgl_get_shared_icon_default_font(), LV_STATE_DEFAULT);
    lv_label_set_text(icon, LVGL_ICON_SHARED_LIST_ALT);
    lv_obj_add_event_cb(button, callback, LV_EVENT_SHORT_CLICKED, ctx);
    return button;
}

void createWidgets(lv_obj_t* parent, void* userData) {
    auto* ctx = static_cast<Context*>(userData);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

    auto* toolbar = lvgl_toolbar_create(parent, "Mesh");
    lvgl_toolbar_set_nav_action(toolbar, LV_SYMBOL_CLOSE, onBackPressed, ctx);

    auto* wrapper = lv_obj_create(parent);
    lv_obj_set_style_border_width(wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);

    auto* card = lvgl_card_create(wrapper);
    lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

    auto* serviceRow = createRow(card);
    ctx->serviceLabel = createTitleAndValue(serviceRow, "Mesh network");
    createChangeButton(serviceRow, onSelectServicePressed, ctx);

    auto* nameRow = createRow(card);
    ctx->nameLabel = createTitleAndValue(nameRow, "Name");
    createChangeButton(nameRow, onChangeNamePressed, ctx);

    auto* radioRow = createRow(card);
    ctx->radioLabel = createTitleAndValue(radioRow, "Radio");
    ctx->radioButton = createChangeButton(radioRow, onSelectRadioPressed, ctx);

    auto* regionRow = createRow(card);
    ctx->regionLabel = createTitleAndValue(regionRow, "Region");
    createChangeButton(regionRow, onSelectRegionPressed, ctx);

    ctx->presetRow = createRow(card);
    ctx->presetLabel = createTitleAndValue(ctx->presetRow, "Preset");
    createChangeButton(ctx->presetRow, onSelectPresetPressed, ctx);

    ctx->channelsRow = createRow(card);
    ctx->channelsLabel = createTitleAndValue(ctx->channelsRow, "Channels");
    auto* addButton = lv_button_create(ctx->channelsRow);
    lv_label_set_text(lv_label_create(addButton), "Join");
    lv_obj_add_event_cb(addButton, onAddChannelPressed, LV_EVENT_SHORT_CLICKED, ctx);

    auto* statusRow = createRow(card);
    ctx->statusLabel = createTitleAndValue(statusRow, "Status");

    updateLabels(ctx);
}

void destroyWidgets(void* userData) {
    auto* ctx = static_cast<Context*>(userData);
    ctx->serviceLabel = nullptr;
    ctx->nameLabel = nullptr;
    ctx->radioButton = nullptr;
    ctx->radioLabel = nullptr;
    ctx->regionLabel = nullptr;
    ctx->presetRow = nullptr;
    ctx->presetLabel = nullptr;
    ctx->channelsRow = nullptr;
    ctx->channelsLabel = nullptr;
    ctx->statusLabel = nullptr;
}

int32_t appMain(int argc, char* argv[]) {
    Context ctx {};
    ctx.appInstanceId = app_scheduler_current_app_id();

    TaskEventGroup eventGroup {};
    task_event_group_construct(&eventGroup);
    ctx.eventGroup = &eventGroup;
    check(task_event_group_claim_bit(&eventGroup, &ctx.selectServiceBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.selectRegionBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.selectPresetBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.addChannelBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.changeNameBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.selectRadioBit) == ERROR_NONE);
    loadState(&ctx);

    AppEventSubscription appEvents {};
    check(app_event_subscribe(&appEvents, &eventGroup) == ERROR_NONE);
    MeshEventSubscription meshEvents {};
    check(mesh_event_subscribe(&meshEvents, &eventGroup) == ERROR_NONE);

    WindowId window = window_manager_create_ext(ctx.appInstanceId, createWidgets, destroyWidgets, &ctx);

    bool shouldClose = false;
    while (!shouldClose) {
        uint32_t flags = 0;
        task_event_group_wait_any(&eventGroup, &flags, portMAX_DELAY);

        if (flags & ctx.selectServiceBit) {
            startServiceSelection(&ctx);
        }
        if (flags & ctx.selectRegionBit) {
            startRegionSelection(&ctx);
        }
        if (flags & ctx.selectPresetBit) {
            startPresetSelection(&ctx);
        }
        if (flags & ctx.addChannelBit) {
            startChannelDialog(&ctx);
        }
        if (flags & ctx.changeNameBit) {
            startNameDialog(&ctx);
        }
        if (flags & ctx.selectRadioBit) {
            startRadioSelection(&ctx);
        }

        bool changed = false;
        MeshEvent meshEvent {};
        while (mesh_event_poll(&meshEvents, &meshEvent) == ERROR_NONE) {
            changed |= meshEvent.type == MESH_EVENT_STATE || meshEvent.type == MESH_EVENT_CHANNEL;
        }

        AppEvent event {};
        while (app_event_poll(&appEvents, &event) == ERROR_NONE) {
            if (event.type == APP_EVENT_CLOSE) {
                shouldClose = true;
                break;
            }
            if (event.type == APP_EVENT_RESULT) {
                if (event.result.launch_id == ctx.serviceDialogId) {
                    ctx.serviceDialogId = 0;
                    onServiceSelected(&ctx, event.result.result);
                } else if (event.result.launch_id == ctx.regionDialogId) {
                    ctx.regionDialogId = 0;
                    onRegionSelected(event.result.result);
                } else if (event.result.launch_id == ctx.presetDialogId) {
                    ctx.presetDialogId = 0;
                    onPresetSelected(&ctx, event.result.result);
                } else if (event.result.launch_id == ctx.channelDialogId) {
                    ctx.channelDialogId = 0;
                    onChannelDialogResult(&ctx, event.result.result);
                } else if (event.result.launch_id == ctx.radioDialogId) {
                    ctx.radioDialogId = 0;
                    onRadioSelected(&ctx, event.result.result);
                } else if (event.result.launch_id == ctx.nameDialogId) {
                    ctx.nameDialogId = 0;
                    onNameDialogResult(&ctx, event.result.result);
                }
                app_manager_stop(event.result.launch_id);
                changed = true;
            }
        }

        if (changed) {
            loadState(&ctx);
            lvgl_lock();
            updateLabels(&ctx);
            lvgl_unlock();
        }
    }

    window_manager_remove(window);
    check(mesh_event_unsubscribe(&meshEvents) == ERROR_NONE);
    check(app_event_unsubscribe(&appEvents) == ERROR_NONE);
    task_event_group_destruct(&eventGroup);
    return 0;
}

} // namespace

/** A new channel gets the next free index, and a channel with the same name keeps its index */
static error_t findChannelIndex(const char* name, uint8_t* index) {
    MeshChannel channels[8];
    size_t count = 0;
    const error_t error = mesh_get_channels(channels, 8, &count);
    if (error != ERROR_NONE) {
        return error;
    }
    *index = 0;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(channels[i].name, name) == 0) {
            *index = channels[i].index;
            return ERROR_NONE;
        }
        *index = std::max<uint8_t>(*index, channels[i].index + 1);
    }
    return ERROR_NONE;
}

error_t joinHashtagChannel(const char* name) {
    if (name[name[0] == '#' ? 1 : 0] == '\0') {
        return ERROR_INVALID_ARGUMENT;
    }

    MeshChannel channel {};
    const error_t error = findChannelIndex(name, &channel.index);
    if (error != ERROR_NONE) {
        return error;
    }
    strncpy(channel.name, name, MESH_NAME_MAX_LENGTH);
    channel.key_length = 0;
    return mesh_set_channel(&channel);
}

/** Sets a channel with a key on the next free index */
static error_t setKeyedChannel(const char* name, const uint8_t* key, size_t keyLength) {
    if (name[0] == '\0' || keyLength == 0 || keyLength > MESH_CHANNEL_KEY_MAX_LENGTH) {
        return ERROR_INVALID_ARGUMENT;
    }

    MeshChannel channel {};
    const error_t error = findChannelIndex(name, &channel.index);
    if (error != ERROR_NONE) {
        return error;
    }
    strncpy(channel.name, name, MESH_NAME_MAX_LENGTH);
    channel.key_length = static_cast<uint8_t>(keyLength);
    memcpy(channel.key, key, keyLength);
    return mesh_set_channel(&channel);
}

error_t createPrivateChannel(const char* name) {
    uint8_t key[16];
#ifdef ESP_PLATFORM
    esp_fill_random(key, sizeof(key));
#else
    std::random_device device;
    for (auto& byte : key) {
        byte = static_cast<uint8_t>(device());
    }
#endif
    return setKeyedChannel(name, key, sizeof(key));
}

error_t joinPrivateChannel(const char* name, const uint8_t* key, size_t keyLength) {
    return setKeyedChannel(name, key, keyLength);
}

extern const ::AppManifest manifest = {
    .id = "tactility.meshsettings",
    .name = "Mesh",
    .category = APP_CATEGORY_SETTINGS,
    .location = { .type = APP_LOCATION_MEMORY, .location = reinterpret_cast<void*>(appMain) },
    .flags = 0,
    .stack = { .depth = 6144, .desired_memory_capability = 0 },
};

} // namespace tt::app::meshsettings

