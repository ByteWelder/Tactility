#if TT_FEATURE_MESH

#include <app/event.h>
#include <app/manifest.h>
#include <app/scheduler.h>

#include <lvgl_window_manager/window_manager.h>

#include <TactilityCpp/std/deque.h>
#include <TactilityCpp/std/map.h>
#include <TactilityCpp/std/string.h>
#include <TactilityCpp/std/vector.h>

#include <mesh/mesh.h>

#include <tactility/check.h>
#include <tactility/concurrent/task_event_group.h>
#include <tactility/concurrent/thread.h>

#include <lvgl/fonts.h>
#include <lvgl/grid_navigation.h>
#include <lvgl/lvgl.h>
#include <lvgl/widgets/focus_container.h>
#include <lvgl/widgets/toolbar.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace tt::app::meshinspect {

extern const ::AppManifest manifest;

namespace {

constexpr size_t ENTRY_LIMIT = 50;
constexpr size_t LOAD_THREAD_STACK_SIZE = 4096;

/** A stored message, or a frame that was only reported live (seq 0) */
struct Entry {
    MeshMessage message;
    bool isFrame;
    MeshFrameKind frameKind;
    uint16_t frameLength;
};

struct Context {
    uint32_t appInstanceId;
    /** Oldest first */
    tt::std::deque<Entry> entries;
    /** Channel names by channel index */
    tt::std::map<uint8_t, tt::std::string> channelNames;
    /** Set until the loaded data is shown. Written by the app task under the LVGL lock */
    bool loading = true;
    /** Delivery updates for messages that may be part of the history that's still loading */
    tt::std::map<uint32_t, MeshDeliveryState> pendingDeliveries;
    TaskEventGroup* eventGroup = nullptr;
    uint32_t loadedBit = 0;
    Thread* loadThread = nullptr;
    ::std::atomic<bool> loadCancelled = false;
    // Written by the load thread until it signals loadedBit
    tt::std::deque<Entry> loadedEntries;
    tt::std::map<uint8_t, tt::std::string> loadedChannelNames;
    // Valid while the window is shown
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* list = nullptr;
    lv_obj_t* emptyLabel = nullptr;
};

const char* getDeliveryText(MeshDeliveryState delivery) {
    switch (delivery) {
        case MESH_DELIVERY_QUEUED:
            return "queued";
        case MESH_DELIVERY_BLOCKED:
            return "blocked";
        case MESH_DELIVERY_SENT:
            return "sent";
        case MESH_DELIVERY_DELIVERED:
            return "delivered";
        case MESH_DELIVERY_FAILED:
            return "failed";
        case MESH_DELIVERY_CANCELLED:
            return "cancelled";
        default:
            return "";
    }
}

::std::string getAddressText(const MeshAddress& address) {
    // The first bytes identify the node well enough to tell nodes apart
    ::std::string text;
    char hex[3];
    for (size_t i = 0; i < address.length && i < 4; i++) {
        snprintf(hex, sizeof(hex), "%02x", address.bytes[i]);
        text += hex;
    }
    return text;
}

::std::string getChannelText(const Context* ctx, uint8_t channel) {
    const auto iterator = ctx->channelNames.find(channel);
    return (iterator != ctx->channelNames.end()) ? ::std::string(iterator->second.c_str()) : ("channel " + ::std::to_string(channel));
}

::std::string getSignalText(float rssi, float snr) {
    char text[40];
    if (::std::isnan(snr)) {
        snprintf(text, sizeof(text), "%.0f dBm", rssi);
    } else {
        snprintf(text, sizeof(text), "%.0f dBm, %.1f dB", rssi, snr);
    }
    return text;
}

::std::string getFrameHeaderText(const Entry& entry) {
    const auto& message = entry.message;
    char header[160];
    if (entry.frameKind == MESH_FRAME_ADVERT) {
        snprintf(header, sizeof(header), "%s < advert %s  %s", message.backend, getAddressText(message.peer).c_str(), getSignalText(message.rssi, message.snr).c_str());
    } else {
        snprintf(header, sizeof(header), "%s < %u bytes  %s", message.backend, (unsigned)entry.frameLength, getSignalText(message.rssi, message.snr).c_str());
    }
    return header;
}

::std::string getHeaderText(const Context* ctx, const Entry& entry) {
    if (entry.isFrame) {
        return getFrameHeaderText(entry);
    }
    const auto& message = entry.message;
    ::std::string target = (message.peer.length == 0) ? getChannelText(ctx, message.channel) : getAddressText(message.peer);
    char header[160];
    if (message.outgoing) {
        snprintf(header, sizeof(header), "%s > %s  [%s]", message.backend, target.c_str(), getDeliveryText(message.delivery));
    } else {
        const char* sender = (message.sender_name[0] != '\0') ? message.sender_name : "?";
        snprintf(header, sizeof(header), "%s < %s %s  %s", message.backend, sender, target.c_str(), getSignalText(message.rssi, message.snr).c_str());
    }
    return header;
}

::std::string getStatusText() {
    char backend[MESH_BACKEND_ID_MAX_LENGTH + 1] = "-";
    mesh_get_backend(backend, sizeof(backend));
    return ::std::string(backend) + ": " + mesh_state_to_string(mesh_get_state());
}

const char* getBodyText(const Entry& entry) {
    if (!entry.isFrame) {
        return entry.message.text;
    }
    if (entry.frameKind == MESH_FRAME_ADVERT) {
        return (entry.message.sender_name[0] != '\0') ? entry.message.sender_name : "[no name]";
    }
    return "[could not read]";
}

/** Requires the LVGL lock */
lv_obj_t* createEntryWidget(Context* ctx, const Entry& entry) {
    const auto& message = entry.message;
    // Shows a focus ring when the keys select it
    auto* item = lvgl_focus_container_create(ctx->list);
    lv_obj_set_size(item, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(item, 4, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(item, 0, LV_STATE_DEFAULT);
    lv_obj_set_scrollable(item, false);
    lv_obj_set_user_data(item, reinterpret_cast<void*>(static_cast<uintptr_t>(message.seq)));

    auto* header = lv_label_create(item);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_style_text_font(header, lvgl_get_text_font(FONT_SIZE_SMALL), LV_STATE_DEFAULT);
    lv_label_set_text(header, getHeaderText(ctx, entry).c_str());

    auto* text = lv_label_create(item);
    lv_obj_set_width(text, LV_PCT(100));
    lv_label_set_long_mode(text, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(text, getBodyText(entry));
    return item;
}

/** Requires the LVGL lock */
void selectEntryWidget(Context* ctx, lv_obj_t* item, lv_obj_t* previous) {
    // The new item has no position until the layout is updated, and removed items leave the scroll position past the end
    lv_obj_update_layout(ctx->list);
    lv_obj_readjust_scroll(ctx->list, LV_ANIM_OFF);
    // Grid navigation selects the first entry by default, which would make the next entries count as browsing
    const bool showRing = previous != nullptr && lv_obj_has_state(previous, LV_STATE_FOCUS_KEY);
    lv_gridnav_set_focused(ctx->list, item, LV_ANIM_OFF);
    if (!showRing) {
        lv_obj_remove_state(item, LV_STATE_FOCUS_KEY);
    }
    if (!lv_obj_has_state(ctx->list, LV_STATE_FOCUSED)) {
        lv_obj_remove_state(item, LV_STATE_FOCUSED);
    }
}

/** Requires the LVGL lock */
void addEntryWidget(Context* ctx, const Entry& entry) {
    // Keep the view on the newest entry, unless the keys selected an older one
    lv_group_t* group = lv_group_get_default();
    lv_obj_t* focused = lvgl_grid_navigation_get_focused(ctx->list);
    const uint32_t childCount = lv_obj_get_child_count(ctx->list);
    const bool browsing = group != nullptr && lv_group_get_focused(group) == ctx->list && focused != nullptr &&
        focused != lv_obj_get_child(ctx->list, (int32_t)childCount - 1);

    auto* item = createEntryWidget(ctx, entry);
    lv_obj_set_hidden(ctx->emptyLabel, true);
    if (browsing) {
        lv_obj_update_layout(ctx->list);
        lv_obj_readjust_scroll(ctx->list, LV_ANIM_OFF);
    } else {
        selectEntryWidget(ctx, item, focused);
    }
}

/** Requires the LVGL lock */
void addAllEntryWidgets(Context* ctx) {
    if (ctx->entries.empty()) {
        return;
    }
    lv_obj_t* focused = lvgl_grid_navigation_get_focused(ctx->list);
    lv_obj_t* item = nullptr;
    for (const auto& entry : ctx->entries) {
        item = createEntryWidget(ctx, entry);
    }
    lv_obj_set_hidden(ctx->emptyLabel, true);
    selectEntryWidget(ctx, item, focused);
}

/** Requires the LVGL lock */
void updateEntryWidget(Context* ctx, const Entry& entry) {
    const auto& message = entry.message;
    const uint32_t count = lv_obj_get_child_count(ctx->list);
    for (uint32_t i = 0; i < count; i++) {
        auto* item = lv_obj_get_child(ctx->list, (int32_t)i);
        if (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(item))) == message.seq) {
            lv_label_set_text(lv_obj_get_child(item, 0), getHeaderText(ctx, entry).c_str());
            return;
        }
    }
}

void onBackPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    app_event_emit_close(ctx->appInstanceId);
}

void createWidgets(lv_obj_t* parent, void* userData) {
    auto* ctx = static_cast<Context*>(userData);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

    auto* toolbar = lvgl_toolbar_create(parent, "Mesh Inspect");
    lvgl_toolbar_set_nav_action(toolbar, LV_SYMBOL_CLOSE, onBackPressed, ctx);

    ctx->statusLabel = lv_label_create(parent);
    lv_obj_set_width(ctx->statusLabel, LV_PCT(100));
    lv_obj_set_style_pad_hor(ctx->statusLabel, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ctx->statusLabel, lvgl_get_text_font(FONT_SIZE_SMALL), LV_STATE_DEFAULT);
    lv_label_set_text(ctx->statusLabel, ctx->loading ? "Loading..." : getStatusText().c_str());

    ctx->list = lv_obj_create(parent);
    lv_obj_set_width(ctx->list, LV_PCT(100));
    lv_obj_set_flex_grow(ctx->list, 1);
    lv_obj_set_flex_flow(ctx->list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(ctx->list, 4, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ctx->list, 0, LV_STATE_DEFAULT);

    ctx->emptyLabel = lv_label_create(ctx->list);
    lv_label_set_text(ctx->emptyLabel, "No messages yet");
    // The arrow keys move between the entries
    lvgl_grid_navigation_add(ctx->list);

    if (!ctx->loading) {
        addAllEntryWidgets(ctx);
    }
}

void destroyWidgets(void* userData) {
    auto* ctx = static_cast<Context*>(userData);
    ctx->statusLabel = nullptr;
    ctx->list = nullptr;
    ctx->emptyLabel = nullptr;
}

/** Queries the service thread, so it's called without the LVGL lock */
void loadChannelNames(tt::std::map<uint8_t, tt::std::string>& channelNames) {
    MeshChannel channels[8];
    size_t count = 0;
    if (mesh_get_channels(channels, 8, &count) != ERROR_NONE) {
        return;
    }
    channelNames.clear();
    for (size_t i = 0; i < count; i++) {
        channelNames[channels[i].index] = channels[i].name;
    }
}

void loadHistory(tt::std::deque<Entry>& entries) {
    tt::std::vector<MeshMessage> history(ENTRY_LIMIT);
    size_t count = 0;
    if (mesh_history_get(0, history.data(), ENTRY_LIMIT, &count) != ERROR_NONE) {
        return;
    }
    // History is newest first
    for (size_t i = count; i > 0; i--) {
        entries.push_back({ .message = history[i - 1], .isFrame = false, .frameKind = MESH_FRAME_UNREADABLE, .frameLength = 0 });
    }
}

void addEntry(Context* ctx, const Entry& entry) {
    ctx->entries.push_back(entry);
    const bool trimmed = ctx->entries.size() > ENTRY_LIMIT;
    if (trimmed) {
        ctx->entries.pop_front();
    }

    if (ctx->loading) {
        return;
    }
    lvgl_lock();
    if (ctx->list != nullptr) {
        if (trimmed) {
            // Index 0 is the "no messages" label
            lv_obj_delete(lv_obj_get_child(ctx->list, 1));
        }
        addEntryWidget(ctx, entry);
    }
    lvgl_unlock();
}

void onMessage(Context* ctx, uint32_t seq) {
    MeshMessage message;
    if (mesh_message_get(seq, &message) == ERROR_NONE) {
        addEntry(ctx, { .message = message, .isFrame = false, .frameKind = MESH_FRAME_UNREADABLE, .frameLength = 0 });
    }
}

void onFrame(Context* ctx, const MeshEvent& event) {
    Entry entry {};
    entry.isFrame = true;
    entry.frameKind = event.frame.kind;
    entry.frameLength = event.frame.length;
    mesh_get_backend(entry.message.backend, sizeof(entry.message.backend));
    entry.message.peer = event.frame.from;
    strncpy(entry.message.sender_name, event.frame.name, MESH_NAME_MAX_LENGTH);
    entry.message.rssi = event.frame.rssi;
    entry.message.snr = event.frame.snr;
    addEntry(ctx, entry);
}

void onDelivery(Context* ctx, uint32_t seq, MeshDeliveryState delivery) {
    for (auto& entry : ctx->entries) {
        if (!entry.isFrame && entry.message.seq == seq) {
            entry.message.delivery = delivery;
            lvgl_lock();
            if (ctx->list != nullptr) {
                updateEntryWidget(ctx, entry);
            }
            lvgl_unlock();
            return;
        }
    }
    if (ctx->loading) {
        ctx->pendingDeliveries[seq] = delivery;
    }
}

/** Queries the service thread, so the window can show while it waits */
int32_t loadThreadMain(void* context) {
    auto* ctx = static_cast<Context*>(context);
    if (!ctx->loadCancelled) {
        loadChannelNames(ctx->loadedChannelNames);
    }
    if (!ctx->loadCancelled) {
        loadHistory(ctx->loadedEntries);
    }
    task_event_group_signal(ctx->eventGroup, ctx->loadedBit);
    return 0;
}

bool startLoading(Context* ctx) {
    ctx->loadThread = thread_alloc();
    if (ctx->loadThread == nullptr) {
        return false;
    }
    thread_set_name(ctx->loadThread, "mesh_inspect_load");
    thread_set_stack_size(ctx->loadThread, LOAD_THREAD_STACK_SIZE);
    thread_set_main_function(ctx->loadThread, loadThreadMain, ctx);
    if (thread_start(ctx->loadThread) != ERROR_NONE) {
        thread_free(ctx->loadThread);
        ctx->loadThread = nullptr;
        return false;
    }
    return true;
}

void joinLoadThread(Context* ctx) {
    if (ctx->loadThread != nullptr) {
        thread_join(ctx->loadThread, portMAX_DELAY, pdMS_TO_TICKS(10));
        thread_free(ctx->loadThread);
        ctx->loadThread = nullptr;
    }
}

/** Puts the history before the entries that arrived while it loaded, then shows them all at once */
void onLoaded(Context* ctx) {
    joinLoadThread(ctx);
    ctx->channelNames = ::std::move(ctx->loadedChannelNames);
    for (auto iterator = ctx->loadedEntries.rbegin(); iterator != ctx->loadedEntries.rend(); ++iterator) {
        bool known = false;
        for (const auto& entry : ctx->entries) {
            if (!entry.isFrame && entry.message.seq == iterator->message.seq) {
                known = true;
                break;
            }
        }
        if (!known) {
            ctx->entries.push_front(*iterator);
        }
    }
    ctx->loadedEntries.clear();
    while (ctx->entries.size() > ENTRY_LIMIT) {
        ctx->entries.pop_front();
    }
    for (auto& entry : ctx->entries) {
        const auto pending = ctx->pendingDeliveries.find(entry.message.seq);
        if (!entry.isFrame && pending != ctx->pendingDeliveries.end()) {
            entry.message.delivery = pending->second;
        }
    }
    ctx->pendingDeliveries.clear();

    lvgl_lock();
    ctx->loading = false;
    if (ctx->list != nullptr) {
        lv_label_set_text(ctx->statusLabel, getStatusText().c_str());
        addAllEntryWidgets(ctx);
    }
    lvgl_unlock();
}

int32_t appMain(int argc, char* argv[]) {
    Context ctx {};
    ctx.appInstanceId = app_scheduler_current_app_id();

    TaskEventGroup eventGroup {};
    task_event_group_construct(&eventGroup);
    ctx.eventGroup = &eventGroup;
    check(task_event_group_claim_bit(&eventGroup, &ctx.loadedBit) == ERROR_NONE);
    AppEventSubscription appEvents {};
    check(app_event_subscribe(&appEvents, &eventGroup) == ERROR_NONE);
    MeshEventSubscription meshEvents {};
    check(mesh_event_subscribe(&meshEvents, &eventGroup) == ERROR_NONE);

    WindowId window = window_manager_create_ext(ctx.appInstanceId, createWidgets, destroyWidgets, &ctx);
    // Shows "Loading..." before the load thread competes for the CPU
    lvgl_lock();
    lv_refr_now(nullptr);
    lvgl_unlock();
    if (!startLoading(&ctx)) {
        task_event_group_signal(&eventGroup, ctx.loadedBit);
    }

    bool shouldClose = false;
    while (!shouldClose) {
        uint32_t flags = 0;
        task_event_group_wait_any(&eventGroup, &flags, portMAX_DELAY);

        if (flags & ctx.loadedBit) {
            onLoaded(&ctx);
        }

        MeshEvent meshEvent {};
        while (mesh_event_poll(&meshEvents, &meshEvent) == ERROR_NONE) {
            switch (meshEvent.type) {
                case MESH_EVENT_MESSAGE:
                    onMessage(&ctx, meshEvent.seq);
                    break;
                case MESH_EVENT_FRAME:
                    onFrame(&ctx, meshEvent);
                    break;
                case MESH_EVENT_DELIVERY:
                    onDelivery(&ctx, meshEvent.seq, meshEvent.delivery);
                    break;
                case MESH_EVENT_STATE:
                    loadChannelNames(ctx.channelNames);
                    lvgl_lock();
                    if (ctx.statusLabel != nullptr && !ctx.loading) {
                        lv_label_set_text(ctx.statusLabel, getStatusText().c_str());
                    }
                    lvgl_unlock();
                    break;
                default:
                    break;
            }
        }

        AppEvent event {};
        while (app_event_poll(&appEvents, &event) == ERROR_NONE) {
            if (event.type == APP_EVENT_CLOSE) {
                shouldClose = true;
                break;
            }
        }
    }

    ctx.loadCancelled = true;
    joinLoadThread(&ctx);
    window_manager_remove(window);
    check(mesh_event_unsubscribe(&meshEvents) == ERROR_NONE);
    check(app_event_unsubscribe(&appEvents) == ERROR_NONE);
    task_event_group_destruct(&eventGroup);
    return 0;
}

} // namespace

extern const ::AppManifest manifest = {
    .id = "tactility.meshinspect",
    .name = "Mesh Inspect",
    .category = APP_CATEGORY_USER,
    .location = { APP_LOCATION_MEMORY, reinterpret_cast<void*>(appMain) },
    .flags = 0,
    .stack = {}
};

} // namespace tt::app::meshinspect

#endif
