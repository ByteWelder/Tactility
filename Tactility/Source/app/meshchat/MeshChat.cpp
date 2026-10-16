#if TT_FEATURE_MESH

#include <Tactility/app/alertdialog/AlertDialog.h>
#include <Tactility/app/inputdialog/InputDialog.h>
#include <Tactility/app/meshchat/MeshChatCommon.h>
#include <Tactility/app/meshsettings/MeshChannels.h>
#include <Tactility/app/qrcode/QrCode.h>

#include <app/event.h>
#include <app/manager.h>
#include <app/manifest.h>
#include <app/scheduler.h>
#include <app/start.h>
#include <app/stream.h>

#include <lvgl_window_manager/window_manager.h>

#include <TactilityCpp/std/vector.h>

#include <mesh/mesh.h>

#include <tactility/check.h>
#include <tactility/concurrent/task_event_group.h>
#include <tactility/log.h>

#include <lvgl/fonts.h>
#include <lvgl/icons/shared.h>
#include <lvgl/lvgl.h>
#include <lvgl/theme.h>
#include <lvgl/widgets/card.h>
#include <lvgl/widgets/list.h>
#include <lvgl/widgets/text_button.h>
#include <lvgl/widgets/toolbar.h>

#include <atomic>
#include <cstring>
#include <string>

namespace tt::app::meshchat {

extern const ::AppManifest manifest;

namespace {

constexpr auto* TAG = "MeshChat";
constexpr size_t CONTACT_LIMIT = 64;
constexpr size_t CHANNEL_LIMIT = 8;

struct Context {
    uint32_t appInstanceId;
    TaskEventGroup* eventGroup = nullptr;
    uint32_t openBit = 0;
    uint32_t settingsBit = 0;
    uint32_t addContactBit = 0;
    uint32_t joinChannelBit = 0;
    uint32_t shareBit = 0;
    uint32_t addByKeyBit = 0;
    /** Written by the LVGL thread before it signals openBit */
    ::std::atomic<bool> openIsContact = true;
    ::std::atomic<int32_t> openIndex = 0;
    uint32_t joinChannelDialogId = 0;
    uint32_t contactNameDialogId = 0;
    uint32_t contactKeyDialogId = 0;
    uint32_t messageDialogId = 0;
    AppStream channelStream {};
    char channelBuffer[MESH_NAME_MAX_LENGTH + 1] = {};
    AppStream contactNameStream {};
    char contactNameBuffer[MESH_NAME_MAX_LENGTH + 1] = {};
    AppStream contactKeyStream {};
    char contactKeyBuffer[MESH_ADDRESS_MAX_LENGTH * 2 + 1] = {};
    /** The name that was entered while the key is asked */
    ::std::string pendingContactName;
    tt::std::vector<MeshContact> contacts;
    tt::std::vector<MeshChannel> channels;
    /** The active tab, kept for when the window is rebuilt. Only the LVGL thread uses it. */
    uint32_t activeTab = 0;
    // Valid while the window is shown
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* contactsTab = nullptr;
    lv_obj_t* channelsTab = nullptr;
};

bool isMeshOn() {
    return mesh_get_state() == MESH_STATE_ON;
}

::std::string getStatusText() {
    return ::std::string("Mesh: ") + mesh_state_to_string(mesh_get_state());
}

/** Queries the service thread, so it's called without the LVGL lock */
void loadData(Context* ctx) {
    ctx->contacts.assign(CONTACT_LIMIT, MeshContact {});
    size_t count = 0;
    if (mesh_get_contacts(ctx->contacts.data(), CONTACT_LIMIT, &count) != ERROR_NONE) {
        count = 0;
    }
    ctx->contacts.resize(count);

    ctx->channels.assign(CHANNEL_LIMIT, MeshChannel {});
    count = 0;
    if (mesh_get_channels(ctx->channels.data(), CHANNEL_LIMIT, &count) != ERROR_NONE) {
        count = 0;
    }
    ctx->channels.resize(count);
}

/** Adds a card that holds a list to a tab. @return the list */
lv_obj_t* createTabList(lv_obj_t* tab) {
    auto* card = lvgl_card_create(tab);
    lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(card, 0, LV_STATE_DEFAULT);
    // The list items' pressed and focused backgrounds follow the card's rounded corners
    lv_obj_set_style_clip_corner(card, true, LV_STATE_DEFAULT);

    // The card provides the background
    auto* list = lvgl_list_create(card);
    lv_obj_set_size(list, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(list, 0, LV_STATE_DEFAULT);
    return list;
}

/** A row of buttons that continues on the next line when they don't fit */
lv_obj_t* createButtonRow(lv_obj_t* parent) {
    auto* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(row, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(row, LV_DPX(4), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(row, LV_DPX(4), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(row, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_scrollable(row, false);
    return row;
}

lv_obj_t* addActionButton(lv_obj_t* parent, const char* text, lv_event_cb_t callback, void* callbackData) {
    auto* button = lvgl_text_button_create(parent);
    // As big as the text, so that as many as possible fit on a line
    lv_obj_set_size(button, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(button, 0);
    lv_obj_add_event_cb(button, callback, LV_EVENT_SHORT_CLICKED, callbackData);
    lv_label_set_text(lv_label_create(button), text);
    return button;
}

lv_obj_t* addListButton(lv_obj_t* list, const char* text, lv_event_cb_t callback, void* callbackData, intptr_t index) {
    auto* button = lvgl_list_add_button(list, nullptr, text);
    lv_obj_set_user_data(button, reinterpret_cast<void*>(index));
    lv_obj_add_event_cb(button, callback, LV_EVENT_SHORT_CLICKED, callbackData);
    return button;
}

void onOpenPressed(lv_event_t* event, bool isContact) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    auto* button = lv_event_get_target_obj(event);
    ctx->openIsContact = isContact;
    ctx->openIndex = (int32_t)reinterpret_cast<intptr_t>(lv_obj_get_user_data(button));
    task_event_group_signal(ctx->eventGroup, ctx->openBit);
}

void onContactPressed(lv_event_t* event) {
    onOpenPressed(event, true);
}

void onChannelPressed(lv_event_t* event) {
    onOpenPressed(event, false);
}

void onAddContactPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->addContactBit);
}

void onShareQrPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->shareBit);
}

void onAddByKeyPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->addByKeyBit);
}

void onJoinChannelPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->joinChannelBit);
}

void onSettingsPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    task_event_group_signal(ctx->eventGroup, ctx->settingsBit);
}

void onTabChanged(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    ctx->activeTab = lv_tabview_get_tab_active(lv_event_get_target_obj(event));
}

void onBackPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    app_event_emit_close(ctx->appInstanceId);
}

/** Requires the LVGL lock */
void updateWidgets(Context* ctx) {
    if (ctx->contactsTab == nullptr) {
        return;
    }
    lv_label_set_text(ctx->statusLabel, getStatusText().c_str());
    lv_obj_set_hidden(ctx->statusLabel, isMeshOn());

    lv_obj_clean(ctx->contactsTab);
    auto* buttonRow = createButtonRow(ctx->contactsTab);
    addActionButton(buttonRow, LV_SYMBOL_PLUS " Add by key", onAddByKeyPressed, ctx);
    addActionButton(buttonRow, LV_SYMBOL_PLUS " Add seen", onAddContactPressed, ctx);
    addActionButton(buttonRow, "Show my QR", onShareQrPressed, ctx);
    auto* contactList = createTabList(ctx->contactsTab);
    for (size_t i = 0; i < ctx->contacts.size(); i++) {
        addListButton(contactList, ctx->contacts[i].name, onContactPressed, ctx, (intptr_t)i);
    }
    if (ctx->contacts.empty()) {
        lvgl_list_add_text(contactList, "No contacts yet. Add a node that you've heard, or wait for a direct message.");
    }

    lv_obj_clean(ctx->channelsTab);
    auto* channelList = createTabList(ctx->channelsTab);
    addListButton(channelList, LV_SYMBOL_PLUS " Join channel", onJoinChannelPressed, ctx, 0);
    for (size_t i = 0; i < ctx->channels.size(); i++) {
        addListButton(channelList, ctx->channels[i].name, onChannelPressed, ctx, (intptr_t)i);
    }
}

/** Adds a tab that shows an icon in the vertical tab bar */
lv_obj_t* createTab(lv_obj_t* tabview, const char* icon) {
    auto* tab = lv_tabview_add_tab(tabview, icon);
    auto* tabBar = lv_tabview_get_tab_bar(tabview);
    auto* tabButton = lv_obj_get_child(tabBar, static_cast<int32_t>(lv_obj_get_child_count(tabBar)) - 1);
    auto* iconFont = lvgl_theme_is_compact() ? lvgl_get_shared_icon_default_font() : lvgl_get_shared_icon_large_font();
    lv_obj_set_style_text_font(tabButton, iconFont, LV_STATE_DEFAULT);
    // Square buttons, so the theme's round selection indicator fits the icon
    const int32_t buttonSize = lv_font_get_line_height(iconFont) + 2 * lv_obj_get_style_pad_top(tabButton, LV_PART_MAIN);
    lv_obj_set_flex_grow(tabButton, 0);
    lv_obj_set_size(tabButton, buttonSize, buttonSize);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(tab, LV_DPX(4), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(tab, LV_DPX(4), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(tab, 0, LV_STATE_DEFAULT);
    return tab;
}

void createWidgets(lv_obj_t* parent, void* userData) {
    auto* ctx = static_cast<Context*>(userData);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

    auto* toolbar = lvgl_toolbar_create(parent, "Mesh Chat");
    lvgl_toolbar_set_nav_action(toolbar, LV_SYMBOL_CLOSE, onBackPressed, ctx);
    lvgl_toolbar_add_text_button_action(toolbar, LV_SYMBOL_SETTINGS, onSettingsPressed, ctx);

    ctx->statusLabel = lv_label_create(parent);
    lv_obj_set_width(ctx->statusLabel, LV_PCT(100));
    lv_obj_set_style_pad_hor(ctx->statusLabel, LV_DPX(8), LV_STATE_DEFAULT);

    auto* wrapper = lv_obj_create(parent);
    lv_obj_set_style_border_width(wrapper, 0, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_style_pad_all(wrapper, 0, LV_STATE_DEFAULT);
    // Space between the screen edges and the rounded tab bar and content
    const int32_t edgeSpace = lvgl_theme_is_compact() ? 2 : LV_DPX(8);
    lv_obj_set_style_pad_hor(wrapper, edgeSpace, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(wrapper, edgeSpace, LV_STATE_DEFAULT);

    auto* tabview = lv_tabview_create(wrapper);
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_LEFT);
    // The first tab is the default one
    ctx->contactsTab = createTab(tabview, LVGL_ICON_SHARED_MAIL);
    ctx->channelsTab = createTab(tabview, LVGL_ICON_SHARED_FORUM);

    // As wide as the buttons, which are spread over the bar's height
    auto* tabBar = lv_tabview_get_tab_bar(tabview);
    lv_obj_set_width(tabBar, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(tabBar, lvgl_theme_is_compact() ? 2 : LV_DPX(12), LV_STATE_DEFAULT);
    lv_obj_set_flex_align(tabBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);


    // Comes back on the tab it was left on
    lv_tabview_set_active(tabview, ctx->activeTab, LV_ANIM_OFF);
    lv_obj_add_event_cb(tabview, onTabChanged, LV_EVENT_VALUE_CHANGED, ctx);

    updateWidgets(ctx);
}

void destroyWidgets(void* userData) {
    auto* ctx = static_cast<Context*>(userData);
    ctx->statusLabel = nullptr;
    ctx->contactsTab = nullptr;
    ctx->channelsTab = nullptr;
}

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

void startConversation(Context* ctx) {
    const auto index = (size_t)ctx->openIndex.load();
    if (ctx->openIsContact) {
        if (index < ctx->contacts.size()) {
            const ::std::string address = addressToHex(ctx->contacts[index].address);
            const char* argv[] = { KIND_CONTACT, address.c_str(), ctx->contacts[index].name };
            startApp(CONVERSATION_APP_ID, 3, argv);
        }
    } else if (index < ctx->channels.size()) {
        const ::std::string channelIndex = ::std::to_string(ctx->channels[index].index);
        const char* argv[] = { KIND_CHANNEL, channelIndex.c_str(), ctx->channels[index].name };
        startApp(CONVERSATION_APP_ID, 3, argv);
    }
}

void showMessage(Context* ctx, const char* title, const char* message) {
    if (ctx->messageDialogId == 0) {
        ctx->messageDialogId = alertdialog::start(ctx->appInstanceId, title, message);
    }
}

void startAddContact() {
    startApp(ADD_CONTACT_APP_ID, 0, nullptr);
}

/** Shows this node as a QR code that other mesh apps can scan */
void showOwnQr(Context* ctx) {
    char uri[256];
    const error_t error = mesh_get_contact_uri(nullptr, uri, sizeof(uri));
    if (error == ERROR_NONE) {
        qrcode::start(uri);
    } else if (error == ERROR_NOT_SUPPORTED) {
        showMessage(ctx, "Not available", "This mesh network can't share a contact as a QR code.");
    } else {
        LOG_E(TAG, "Failed to get the contact: %s", error_to_string(error));
        showMessage(ctx, "Couldn't create QR code", error == ERROR_INVALID_STATE ? "Turn mesh on first." : error_to_string(error));
    }
}

void startContactNameDialog(Context* ctx) {
    if (ctx->contactNameDialogId != 0 || ctx->contactKeyDialogId != 0) {
        return;
    }
    ctx->contactNameDialogId = inputdialog::start(
        ctx->appInstanceId,
        "Add contact",
        "Name of the contact",
        "",
        ctx->contactNameStream,
        ctx->contactNameBuffer,
        sizeof(ctx->contactNameBuffer),
        ctx->eventGroup
    );
}

void onContactNameResult(Context* ctx, int32_t result) {
    if (result == 0) {
        char name[sizeof(Context::contactNameBuffer)] = {};
        const size_t length = app_stream_read(&ctx->contactNameStream, name, sizeof(name) - 1);
        name[length] = '\0';
        ctx->pendingContactName = name;
    }
    app_stream_unsubscribe(&ctx->contactNameStream);
    if (result != 0 || ctx->pendingContactName.empty()) {
        ctx->pendingContactName.clear();
        return;
    }
    ctx->contactKeyDialogId = inputdialog::start(
        ctx->appInstanceId,
        "Public key",
        "Public key of the contact in hexadecimal",
        "",
        ctx->contactKeyStream,
        ctx->contactKeyBuffer,
        sizeof(ctx->contactKeyBuffer),
        ctx->eventGroup
    );
}

void onContactKeyResult(Context* ctx, int32_t result) {
    if (result == 0) {
        char key[sizeof(Context::contactKeyBuffer)] = {};
        const size_t length = app_stream_read(&ctx->contactKeyStream, key, sizeof(key) - 1);
        key[length] = '\0';
        MeshAddress address;
        if (!hexToAddress(key, &address)) {
            showMessage(ctx, "Invalid key", "Enter the public key as hexadecimal characters, two for every byte.");
        } else {
            const error_t error = mesh_add_contact(&address, ctx->pendingContactName.c_str());
            if (error == ERROR_INVALID_ARGUMENT) {
                showMessage(ctx, "Invalid key", "This mesh network expects a public key of a different length.");
            } else if (error != ERROR_NONE) {
                LOG_E(TAG, "Failed to add contact: %s", error_to_string(error));
                showMessage(ctx, "Couldn't add contact", error == ERROR_RESOURCE ? "There are too many contacts." : error_to_string(error));
            }
        }
    }
    app_stream_unsubscribe(&ctx->contactKeyStream);
    ctx->pendingContactName.clear();
}

void startJoinChannel(Context* ctx) {
    if (ctx->joinChannelDialogId != 0) {
        return;
    }
    ctx->joinChannelDialogId = inputdialog::start(
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

void onJoinChannelResult(Context* ctx, int32_t result) {
    if (result == 0) {
        char name[sizeof(Context::channelBuffer)] = {};
        const size_t length = app_stream_read(&ctx->channelStream, name, sizeof(name) - 1);
        name[length] = '\0';
        const error_t error = meshsettings::joinHashtagChannel(name);
        if (error == ERROR_INVALID_ARGUMENT) {
            showMessage(ctx, "Couldn't join channel", "Use a channel name that starts with #. This mesh network can't join a channel by name.");
        } else if (error != ERROR_NONE) {
            LOG_E(TAG, "Failed to join channel %s: %s", name, error_to_string(error));
            showMessage(ctx, "Couldn't join channel", error_to_string(error));
        }
    }
    app_stream_unsubscribe(&ctx->channelStream);
}

int32_t appMain(int argc, char* argv[]) {
    Context ctx {};
    ctx.appInstanceId = app_scheduler_current_app_id();

    TaskEventGroup eventGroup {};
    task_event_group_construct(&eventGroup);
    ctx.eventGroup = &eventGroup;
    check(task_event_group_claim_bit(&eventGroup, &ctx.openBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.settingsBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.addContactBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.joinChannelBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.shareBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.addByKeyBit) == ERROR_NONE);

    AppEventSubscription appEvents {};
    check(app_event_subscribe(&appEvents, &eventGroup) == ERROR_NONE);
    MeshEventSubscription meshEvents {};
    check(mesh_event_subscribe(&meshEvents, &eventGroup) == ERROR_NONE);

    loadData(&ctx);
    WindowId window = window_manager_create_ext(ctx.appInstanceId, createWidgets, destroyWidgets, &ctx);

    bool shouldClose = false;
    while (!shouldClose) {
        uint32_t flags = 0;
        task_event_group_wait_any(&eventGroup, &flags, portMAX_DELAY);

        if (flags & ctx.openBit) {
            startConversation(&ctx);
        }
        if (flags & ctx.settingsBit) {
            startApp("tactility.meshsettings", 0, nullptr);
        }
        if (flags & ctx.addContactBit) {
            startAddContact();
        }
        if (flags & ctx.addByKeyBit) {
            startContactNameDialog(&ctx);
        }
        if (flags & ctx.shareBit) {
            showOwnQr(&ctx);
        }
        if (flags & ctx.joinChannelBit) {
            startJoinChannel(&ctx);
        }

        bool changed = false;
        MeshEvent meshEvent {};
        while (mesh_event_poll(&meshEvents, &meshEvent) == ERROR_NONE) {
            changed |= meshEvent.type == MESH_EVENT_STATE || meshEvent.type == MESH_EVENT_CONTACT;
        }

        AppEvent event {};
        while (app_event_poll(&appEvents, &event) == ERROR_NONE) {
            if (event.type == APP_EVENT_CLOSE) {
                shouldClose = true;
                break;
            }
            if (event.type == APP_EVENT_RESULT) {
                if (event.result.launch_id == ctx.contactNameDialogId) {
                    ctx.contactNameDialogId = 0;
                    onContactNameResult(&ctx, event.result.result);
                } else if (event.result.launch_id == ctx.contactKeyDialogId) {
                    ctx.contactKeyDialogId = 0;
                    onContactKeyResult(&ctx, event.result.result);
                } else if (event.result.launch_id == ctx.joinChannelDialogId) {
                    ctx.joinChannelDialogId = 0;
                    onJoinChannelResult(&ctx, event.result.result);
                } else if (event.result.launch_id == ctx.messageDialogId) {
                    ctx.messageDialogId = 0;
                }
                app_manager_stop(event.result.launch_id);
                // The channels and contacts may have changed, and the window may have been rebuilt
                changed = true;
            }
        }

        if (changed) {
            loadData(&ctx);
            lvgl_lock();
            updateWidgets(&ctx);
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

extern const ::AppManifest manifest = {
    .id = "tactility.meshchat",
    .name = "Mesh Chat",
    .category = APP_CATEGORY_USER,
    .location = { APP_LOCATION_MEMORY, reinterpret_cast<void*>(appMain) },
    .flags = 0,
    .stack = {}
};

} // namespace tt::app::meshchat

#endif
