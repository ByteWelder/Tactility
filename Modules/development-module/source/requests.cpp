// SPDX-License-Identifier: Apache-2.0
#include <development/requests.h>

#include <tactility/filesystem/fs.h>
#include <tactility/log.h>
#include <tactility/memory.h>

#include <cstring>

namespace {

constexpr auto* TAG = "DevelopmentRequests";

// Bounds the wait for socket input, which doesn't signal the event group
constexpr uint32_t POLL_INTERVAL_MS = 20;
constexpr size_t RECEIVE_BUFFER_SIZE = DEVELOPMENT_FRAME_HEADER_SIZE + DEVELOPMENT_FRAME_PAYLOAD_MAX;

constexpr MemoryPolicy MEMORY_POLICY = { .required = 0, .desired = MEMORY_CAPABILITY_EXTERNAL, .alignment = 0 };

struct Buffers {
    uint8_t receive[RECEIVE_BUFFER_SIZE];
    uint8_t scratch[DEVELOPMENT_FRAME_PAYLOAD_MAX];
};

bool createParentDirectory(const char* path) {
    char parent[FILE_MAX_PATH_STRING_LENGTH];
    strcpy(parent, path);
    char* separator = strrchr(parent, '/');
    if (separator == nullptr || separator == parent) {
        return true;
    }
    *separator = '\0';
    if (directory_exists(parent)) {
        return true;
    }
    const error_t error = directory_make(parent, true);
    return error == ERROR_NONE || error == ERROR_ALREADY_EXISTS;
}

void writePush(DevelopmentRequestContext& context, const DevelopmentFrame& frame) {
    if (context.pushFile != nullptr && fwrite(frame.payload, 1, frame.length, context.pushFile) != frame.length) {
        context.pushError = "failed to write file";
    }
}

/** Closes the pushed file, and removes it when the push failed or the file is temporary */
void closePush(DevelopmentRequestContext& context) {
    if (context.pushFile != nullptr) {
        if (fclose(context.pushFile) != 0 && context.pushError == nullptr) {
            context.pushError = "failed to write file";
        }
        context.pushFile = nullptr;
        if (context.pushError == nullptr && context.pushComplete != nullptr) {
            context.pushError = context.pushComplete(context, context.pushPath);
        }
        if (context.pushError != nullptr || context.pushIsTemporary) {
            remove(context.pushPath);
        }
    }
    context.pushing = false;
}

bool finishPush(DevelopmentRequestContext& context) {
    closePush(context);
    if (context.pushError != nullptr) {
        LOG_W(TAG, "Push to %s failed: %s", context.pushPath, context.pushError);
        return development_requests_send_fail(context, context.pushError);
    }
    LOG_I(TAG, "Pushed %s", context.pushPath);
    return development_requests_send_ok(context);
}

bool handleFrame(DevelopmentRequestContext& context, const DevelopmentFrame& frame, DevelopmentRequestMode mode) {
    if (context.pushing) {
        switch (frame.type) {
            case DEVELOPMENT_FRAME_DATA:
                writePush(context, frame);
                return true;
            case DEVELOPMENT_FRAME_DONE:
                return finishPush(context);
            default:
                return false;
        }
    }
    switch (mode) {
        case DevelopmentRequestMode::Sync:
            return development_sync_handle(context, frame);
        case DevelopmentRequestMode::App:
            return development_app_handle(context, frame);
    }
    return false;
}

/** @return false when the connection must close */
bool processFrames(DevelopmentRequestContext& context, Buffers* buffers, size_t& receiveLength, DevelopmentRequestMode mode) {
    size_t offset = 0;
    bool connected = true;
    while (connected) {
        DevelopmentFrame frame {};
        size_t consumed = 0;
        const auto result = development_frame_parse(buffers->receive + offset, receiveLength - offset, &frame, &consumed);
        if (result == DevelopmentFrameParseResult::Incomplete) {
            break;
        }
        connected = result == DevelopmentFrameParseResult::Ok && handleFrame(context, frame, mode);
        offset += consumed;
    }
    memmove(buffers->receive, buffers->receive + offset, receiveLength - offset);
    receiveLength -= offset;
    return connected;
}

} // namespace

void development_requests_run(DevelopmentSession* session, TaskEventGroup* event_group, AppEventSubscription* subscription, bool* close_requested, DevelopmentRequestMode mode) {
    auto* buffers = static_cast<Buffers*>(memory_alloc_with_policy(sizeof(Buffers), &MEMORY_POLICY));
    if (buffers == nullptr) {
        LOG_E(TAG, "Out of memory");
        return;
    }
    DevelopmentRequestContext context { session, subscription, close_requested, buffers->scratch };
    size_t receiveLength = 0;

    bool connected = true;
    while (connected && !*close_requested) {
        const int result = mbedtls_ssl_read(&session->ssl, buffers->receive + receiveLength, RECEIVE_BUFFER_SIZE - receiveLength);
        if (result > 0) {
            receiveLength += static_cast<size_t>(result);
            connected = processFrames(context, buffers, receiveLength, mode);
        } else if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE) {
            task_event_group_wait_any(event_group, nullptr, pdMS_TO_TICKS(POLL_INTERVAL_MS));
        } else {
            connected = false;
        }
        development_requests_poll_events(context);
    }

    if (context.pushing) {
        context.pushError = "interrupted";
        closePush(context);
    }
    memory_free(buffers);
}

void development_requests_start_push(DevelopmentRequestContext& context, const char* path, const char* error, DevelopmentPushCompleteFn complete, bool temporary) {
    context.pushing = true;
    context.pushFile = nullptr;
    context.pushError = error;
    context.pushComplete = complete;
    context.pushIsTemporary = temporary;
    context.pushPath[0] = '\0';
    if (error != nullptr) {
        return;
    }
    strcpy(context.pushPath, path);
    if (directory_exists(path)) {
        context.pushError = "is a directory";
    } else if (!createParentDirectory(path)) {
        context.pushError = "failed to create parent directory";
    } else if ((context.pushFile = fopen(path, "wb")) == nullptr) {
        context.pushError = "failed to open file";
    }
}

bool development_requests_read_string(const DevelopmentFrame& frame, char* out_string, size_t out_size) {
    if (frame.length == 0 || frame.length >= out_size || memchr(frame.payload, '\0', frame.length) != nullptr) {
        return false;
    }
    memcpy(out_string, frame.payload, frame.length);
    out_string[frame.length] = '\0';
    return true;
}

bool development_requests_read_path(const DevelopmentFrame& frame, char* out_path) {
    return frame.length > 0 && frame.payload[0] == '/' && development_requests_read_string(frame, out_path, FILE_MAX_PATH_STRING_LENGTH);
}

bool development_requests_send_ok(DevelopmentRequestContext& context) {
    return development_session_send_frame(context.session, DEVELOPMENT_FRAME_OK, nullptr, 0);
}

bool development_requests_send_fail(DevelopmentRequestContext& context, const char* message) {
    return development_session_send_frame(context.session, DEVELOPMENT_FRAME_FAIL, message, static_cast<uint16_t>(strlen(message)));
}

void development_requests_poll_events(DevelopmentRequestContext& context) {
    AppEvent event {};
    while (app_event_poll(context.subscription, &event) == ERROR_NONE) {
        if (event.type == APP_EVENT_CLOSE) {
            *context.closeRequested = true;
        }
    }
}
