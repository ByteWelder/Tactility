#include <Tactility/app/terminal/Terminal.h>

#include <Tactility/file/File.h>

#include <app/event.h>
#include <app/execute.h>
#include <app/manager.h>
#include <app/manifest.h>
#include <app/scheduler.h>
#include <app/start.h>
#include <app/stream.h>

#include <lvgl_window_manager/window_manager.h>

#include <lvgl.h>
#include <lvgl/fonts.h>
#include <lvgl/lvgl.h>
#include <lvgl/widgets/toolbar.h>

#include <tactility/check.h>
#include <tactility/log.h>

#include <cstring>
#include <string>
#include <unistd.h>

namespace tt::app::terminal {

constexpr auto* TAG = "Terminal";

extern const ::AppManifest manifest;

namespace {

/**
 * Output retained for display. Once full, whole lines are dropped from the front, which keeps
 * `Context::text` a contiguous NUL-terminated string that lv_label_set_text_static() can point
 * straight at. That is what keeps the output stored exactly once, with no second copy for
 * rendering.
 */
constexpr size_t TEXT_CAPACITY = 4096;
constexpr size_t STREAM_BUFFER_CAPACITY = 512;

/** CSI sequences can arrive split across reads, so this state has to outlive a single chunk. */
enum class EscapeState {
    None,
    Escape,
    Csi,
};

struct Context {
    uint32_t appInstanceId = 0;
    TaskEventGroup* eventGroup = nullptr;

    std::string executablePath;

    AppInstanceId childId = 0;
    bool childRunning = false;
    /** Whether the two streams below were ever bound; they are only valid to unsubscribe if so. */
    bool streamsBound = false;
    AppStream childStdout {};
    uint8_t childStdoutBuffer[STREAM_BUFFER_CAPACITY] {};
    AppStream childStdin {};
    uint8_t childStdinBuffer[STREAM_BUFFER_CAPACITY] {};

    // Everything below is only touched while holding lvgl_lock(): the output label points straight
    // at `text`, so the LVGL task may be reading it while the app task appends to it.
    char text[TEXT_CAPACITY] = "";
    size_t textLength = 0;
    EscapeState escapeState = EscapeState::None;

    // NULL whenever this window isn't the topmost one; see destroyWidgets().
    lv_obj_t* outputContainer = nullptr;
    lv_obj_t* outputLabel = nullptr;
    lv_obj_t* inputTextarea = nullptr;
};

/** Drops whole lines from the front until @a required bytes fit. */
void trimToFit(Context* ctx, size_t required) {
    size_t available = TEXT_CAPACITY - 1 - ctx->textLength;
    if (required <= available) {
        return;
    }

    size_t needed = required - available;
    size_t drop = 0;
    while (drop < needed) {
        const void* newline = memchr(ctx->text + drop, '\n', ctx->textLength - drop);
        if (newline == nullptr) {
            // A single line longer than the whole buffer: drop what's buffered rather than stall.
            drop = ctx->textLength;
            break;
        }
        drop = static_cast<size_t>(static_cast<const char*>(newline) - ctx->text) + 1;
    }

    if (drop >= ctx->textLength) {
        ctx->textLength = 0;
    } else {
        memmove(ctx->text, ctx->text + drop, ctx->textLength - drop);
        ctx->textLength -= drop;
    }
    ctx->text[ctx->textLength] = '\0';
}

void appendChar(Context* ctx, char character) {
    trimToFit(ctx, 1);
    if (ctx->textLength + 1 >= TEXT_CAPACITY) {
        return;
    }
    ctx->text[ctx->textLength++] = character;
    ctx->text[ctx->textLength] = '\0';
}

/** Feeds one output byte through the escape-sequence filter into `text`. */
void consumeByte(Context* ctx, char character) {
    switch (ctx->escapeState) {
        case EscapeState::Escape:
            ctx->escapeState = (character == '[') ? EscapeState::Csi : EscapeState::None;
            return;
        case EscapeState::Csi:
            // A CSI sequence runs until its final byte, which is the first one in 0x40..0x7E.
            if (character >= 0x40 && character <= 0x7E) {
                ctx->escapeState = EscapeState::None;
            }
            return;
        case EscapeState::None:
            break;
    }

    if (character == 0x1B) {
        ctx->escapeState = EscapeState::Escape;
        return;
    }

    if (character == '\r') {
        return; // Both a bare CR and the CR of a CRLF collapse to nothing.
    }

    if (character == '\b') {
        if (ctx->textLength > 0 && ctx->text[ctx->textLength - 1] != '\n') {
            ctx->text[--ctx->textLength] = '\0';
        }
        return;
    }

    // Drop the remaining control characters; UTF-8 continuation bytes are >= 0x80 so they pass.
    if (character != '\n' && character != '\t' && static_cast<unsigned char>(character) < 0x20) {
        return;
    }

    appendChar(ctx, character);
}

/** @warning Caller must hold lvgl_lock(). */
void refreshOutput(Context* ctx) {
    if (ctx->outputLabel == nullptr) {
        return;
    }

    // Re-pointing at the same buffer is what triggers lv_label_refr_text(); lv_obj_invalidate()
    // alone would redraw the label with stale geometry.
    lv_label_set_text_static(ctx->outputLabel, ctx->text);

    if (ctx->outputContainer != nullptr) {
        int32_t bottom = lv_obj_get_scroll_bottom(ctx->outputContainer);
        int32_t current = lv_obj_get_scroll_y(ctx->outputContainer);
        lv_obj_scroll_to_y(ctx->outputContainer, current + bottom, LV_ANIM_OFF);
    }
}

/** Writes a terminal-generated message, as opposed to child output, into the view. */
void appendStatus(Context* ctx, const std::string& message) {
    lvgl_lock();
    for (char character : message) {
        consumeByte(ctx, character);
    }
    consumeByte(ctx, '\n');
    refreshOutput(ctx);
    lvgl_unlock();
}

void drainChildOutput(Context* ctx) {
    if (!ctx->streamsBound) {
        return;
    }

    char chunk[128];
    bool received = false;

    lvgl_lock();
    size_t read;
    while ((read = app_stream_read(&ctx->childStdout, chunk, sizeof(chunk))) > 0) {
        for (size_t i = 0; i < read; i++) {
            consumeByte(ctx, chunk[i]);
        }
        received = true;
    }
    if (received) {
        refreshOutput(ctx);
    }
    lvgl_unlock();
}

void startChild(Context* ctx) {
    if (ctx->executablePath.empty()) {
        appendStatus(ctx, "No executable specified.");
        return;
    }

    AppLocation location { APP_LOCATION_PATH, const_cast<char*>(ctx->executablePath.c_str()) };
    if (!app_is_executable(location)) {
        appendStatus(ctx, "Not executable: " + ctx->executablePath);
        return;
    }

    AppStreamBinding bindings[] = {
        {
            .producer_fd = STDOUT_FILENO,
            .stream = &ctx->childStdout,
            .buffer = ctx->childStdoutBuffer,
            .buffer_capacity = sizeof(ctx->childStdoutBuffer),
            .event_group = ctx->eventGroup,
        },
        {
            .producer_fd = STDIN_FILENO,
            .stream = &ctx->childStdin,
            .buffer = ctx->childStdinBuffer,
            .buffer_capacity = sizeof(ctx->childStdinBuffer),
            .event_group = ctx->eventGroup,
        },
    };

    error_t error = app_execute_for_result_with_streams(location, AppStackConfig {}, 0, nullptr, bindings, 2, ctx->appInstanceId, &ctx->childId);
    if (error != ERROR_NONE) {
        LOG_E(TAG, "Failed to run %s: %s", ctx->executablePath.c_str(), error_to_string(error));
        appendStatus(ctx, std::string("Failed to run: ") + error_to_string(error));
        return;
    }

    ctx->streamsBound = true;
    ctx->childRunning = true;
    LOG_I(TAG, "Running %s as instance %u", ctx->executablePath.c_str(), ctx->childId);
}

void onInputReady(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    if (ctx->inputTextarea == nullptr) {
        return;
    }

    std::string line = lv_textarea_get_text(ctx->inputTextarea);
    line += '\n';

    // This callback runs on the LVGL task with the lock held, so `text` may be touched directly.
    // The child doesn't echo its stdin back, so the line is echoed here to stay visible.
    for (char character : line) {
        consumeByte(ctx, character);
    }
    refreshOutput(ctx);
    lv_textarea_set_text(ctx->inputTextarea, "");

    if (ctx->childRunning) {
        app_stream_write(&ctx->childStdin, line.c_str(), line.length());
    }
}

void createWidgets(lv_obj_t* parent, void* userData) {
    auto* ctx = static_cast<Context*>(userData);

    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

    std::string title = ctx->executablePath.empty() ? "Terminal" : file::getLastPathSegment(ctx->executablePath);
    lv_obj_t* toolbar = lvgl_toolbar_create(parent, title.c_str());
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    ctx->outputContainer = lv_obj_create(parent);
    lv_obj_set_width(ctx->outputContainer, LV_PCT(100));
    lv_obj_set_flex_grow(ctx->outputContainer, 1);
    lv_obj_set_style_pad_all(ctx->outputContainer, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(ctx->outputContainer, 0, LV_PART_MAIN);

    ctx->outputLabel = lv_label_create(ctx->outputContainer);
    lv_obj_set_width(ctx->outputLabel, LV_PCT(100));
    // The default font is proportional, so LVGL does the line breaking. A fixed-width font would
    // instead allow LV_LABEL_LONG_MODE_CLIP with the wrapping done here, against a column count.
    lv_label_set_long_mode(ctx->outputLabel, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_font(ctx->outputLabel, lvgl_get_text_font(FONT_SIZE_SMALL), LV_PART_MAIN);

    ctx->inputTextarea = lv_textarea_create(parent);
    lv_obj_set_width(ctx->inputTextarea, LV_PCT(100));
    lv_textarea_set_one_line(ctx->inputTextarea, true);
    lv_textarea_set_placeholder_text(ctx->inputTextarea, "Input");
    lv_obj_add_event_cb(ctx->inputTextarea, onInputReady, LV_EVENT_READY, ctx);
    if (lv_group_get_default() != nullptr) {
        lv_group_focus_obj(ctx->inputTextarea);
    }

    // Rebuilds the view from the app-owned buffer, which is what makes a resurfaced window show
    // the output it had before it was buried.
    refreshOutput(ctx);
}

void destroyWidgets(void* userData) {
    auto* ctx = static_cast<Context*>(userData);
    // Only the cached pointers may be touched here; see WindowDestroyWidgetsFn's warning about
    // acquiring any other lock from this callback.
    ctx->outputContainer = nullptr;
    ctx->outputLabel = nullptr;
    ctx->inputTextarea = nullptr;
}

int32_t appMain(int argc, char* argv[]) {
    uint32_t appInstanceId = app_scheduler_current_app_id();

    Context ctx {};
    ctx.appInstanceId = appInstanceId;
    if (argc > 0 && argv[0][0] != '\0') {
        ctx.executablePath = argv[0];
    }

    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);
    ctx.eventGroup = &event_group;

    AppEventSubscription sub {};
    check(app_event_subscribe(&sub, &event_group) == ERROR_NONE);

    WindowId window = window_manager_create_ext(appInstanceId, createWidgets, destroyWidgets, &ctx);

    startChild(&ctx);

    bool shouldClose = false;
    while (!shouldClose) {
        // Both streams signal readiness on this same group, so this one wait covers app events
        // and child output alike.
        task_event_group_wait_any(&event_group, nullptr, portMAX_DELAY);

        drainChildOutput(&ctx);

        AppEvent event {};
        while (app_event_poll(&sub, &event) == ERROR_NONE) {
            switch (event.type) {
                case APP_EVENT_CLOSE:
                    shouldClose = true;
                    break;
                case APP_EVENT_RESULT:
                    if (event.result.launch_id == ctx.childId) {
                        // Whatever the child wrote just before exiting is still buffered.
                        drainChildOutput(&ctx);
                        appendStatus(&ctx, "[exited with " + std::to_string(event.result.result) + "]");
                        app_manager_stop(ctx.childId);
                        ctx.childRunning = false;
                    }
                    break;
                default:
                    break;
            }
            if (shouldClose) break;
        }
    }

    if (ctx.childRunning) {
        app_manager_stop(ctx.childId);
        ctx.childRunning = false;
    }
    if (ctx.streamsBound) {
        app_stream_unsubscribe(&ctx.childStdout);
        app_stream_unsubscribe(&ctx.childStdin);
        ctx.streamsBound = false;
    }

    window_manager_remove(window);
    check(app_event_unsubscribe(&sub) == ERROR_NONE);
    task_event_group_destruct(&event_group);

    return 0;
}

} // namespace

void start(const std::string& executablePath) {
    const char* argv[] = { executablePath.c_str() };
    uint32_t instanceId = 0;
    app_start(manifest.id, 1, argv, &instanceId);
}

extern const ::AppManifest manifest = {
    .id = "tactility.terminal",
    .name = "Terminal",
    .category = APP_CATEGORY_SYSTEM,
    .location = { APP_LOCATION_MEMORY, reinterpret_cast<void*>(appMain) },
    .flags = APP_MANIFEST_FLAG_HIDDEN,
};

} // namespace tt::app::terminal
