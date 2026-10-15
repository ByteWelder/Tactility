// SPDX-License-Identifier: Apache-2.0
#include <development/requests.h>

#include <tactility/filesystem/fs.h>
#include <tactility/log.h>

#include <sys/stat.h>

namespace {

constexpr auto* TAG = "DevelopmentSync";

bool handleStat(DevelopmentRequestContext& context, const DevelopmentFrame& frame) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (!development_requests_read_path(frame, path)) {
        return development_requests_send_fail(context, "invalid path");
    }
    uint8_t result[9] = {};
    struct stat info {};
    if (stat(path, &info) == 0) {
        result[0] = S_ISDIR(info.st_mode) ? DEVELOPMENT_SYNC_FILE_DIRECTORY : DEVELOPMENT_SYNC_FILE_REGULAR;
        const auto size = static_cast<uint64_t>(info.st_size);
        for (int i = 0; i < 8; i++) {
            result[1 + i] = static_cast<uint8_t>(size >> (56 - i * 8));
        }
    }
    return development_session_send_frame(context.session, DEVELOPMENT_FRAME_SYNC_STAT_RESULT, result, sizeof(result));
}

void handlePush(DevelopmentRequestContext& context, const DevelopmentFrame& frame) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (development_requests_read_path(frame, path)) {
        development_requests_start_push(context, path, nullptr, nullptr, false);
    } else {
        development_requests_start_push(context, nullptr, "invalid path", nullptr, false);
    }
}

bool handlePull(DevelopmentRequestContext& context, const DevelopmentFrame& frame) {
    char path[FILE_MAX_PATH_STRING_LENGTH];
    if (!development_requests_read_path(frame, path)) {
        return development_requests_send_fail(context, "invalid path");
    }
    if (directory_exists(path)) {
        return development_requests_send_fail(context, "is a directory");
    }
    FILE* file = fopen(path, "rb");
    if (file == nullptr) {
        return development_requests_send_fail(context, "failed to open file");
    }
    bool connected = true;
    size_t length;
    while (connected && (length = fread(context.buffer, 1, DEVELOPMENT_FRAME_PAYLOAD_MAX, file)) > 0) {
        connected = development_session_send_frame(context.session, DEVELOPMENT_FRAME_DATA, context.buffer, static_cast<uint16_t>(length));
        development_requests_poll_events(context);
        if (*context.closeRequested) {
            connected = false;
        }
    }
    const bool failed = ferror(file) != 0;
    fclose(file);
    if (!connected) {
        return false;
    }
    if (failed) {
        return development_requests_send_fail(context, "failed to read file");
    }
    LOG_I(TAG, "Pulled %s", path);
    return development_session_send_frame(context.session, DEVELOPMENT_FRAME_DONE, nullptr, 0);
}

} // namespace

bool development_sync_handle(DevelopmentRequestContext& context, const DevelopmentFrame& frame) {
    switch (frame.type) {
        case DEVELOPMENT_FRAME_SYNC_STAT:
            return handleStat(context, frame);
        case DEVELOPMENT_FRAME_SYNC_PUSH:
            handlePush(context, frame);
            return true;
        case DEVELOPMENT_FRAME_SYNC_PULL:
            return handlePull(context, frame);
        default:
            return false;
    }
}
