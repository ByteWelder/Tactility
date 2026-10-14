// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <development/protocol.h>
#include <development/session.h>

#include <app/event.h>

#include <tactility/concurrent/task_event_group.h>
#include <tactility/paths.h>

#include <cstdio>

enum class DevelopmentRequestMode {
    Sync,
    App,
};

struct DevelopmentRequestContext;

/**
 * Called when a push received its file completely and closed it.
 * @return an error message, or nullptr on success
 */
typedef const char* (*DevelopmentPushCompleteFn)(DevelopmentRequestContext& context, const char* path);

struct DevelopmentRequestContext {
    DevelopmentSession* session;
    AppEventSubscription* subscription;
    bool* closeRequested;
    /** Scratch space for handlers, DEVELOPMENT_FRAME_PAYLOAD_MAX bytes */
    uint8_t* buffer;
    // A push keeps consuming its DATA frames until DONE, also when it failed, and then reports its result
    bool pushing = false;
    FILE* pushFile = nullptr;
    const char* pushError = nullptr;
    char pushPath[FILE_MAX_PATH_STRING_LENGTH] = {};
    DevelopmentPushCompleteFn pushComplete = nullptr;
    bool pushIsTemporary = false;
};

/**
 * Serves requests of @a mode until the client disconnects or the app is asked to close.
 * @param[in,out] close_requested set when an APP_EVENT_CLOSE arrives
 */
void development_requests_run(DevelopmentSession* session, TaskEventGroup* event_group, AppEventSubscription* subscription, bool* close_requested, DevelopmentRequestMode mode);

/**
 * Starts receiving a file into @a path. The result is sent when the client sends DONE.
 * @param[in] error when not nullptr, the push fails with this message
 * @param[in] complete optional, see DevelopmentPushCompleteFn
 * @param[in] temporary removes the file after @a complete, also on success
 */
void development_requests_start_push(DevelopmentRequestContext& context, const char* path, const char* error, DevelopmentPushCompleteFn complete, bool temporary);

/** Copies the frame payload as a null-terminated string. @return false when it's empty, too long or contains a null character */
bool development_requests_read_string(const DevelopmentFrame& frame, char* out_string, size_t out_size);

/** Like development_requests_read_string(), but also requires an absolute path */
bool development_requests_read_path(const DevelopmentFrame& frame, char* out_path);

bool development_requests_send_ok(DevelopmentRequestContext& context);

bool development_requests_send_fail(DevelopmentRequestContext& context, const char* message);

/** Handles APP_EVENT_CLOSE, for handlers that run for a long time */
void development_requests_poll_events(DevelopmentRequestContext& context);

/** @return false when the connection must close */
bool development_sync_handle(DevelopmentRequestContext& context, const DevelopmentFrame& frame);

/** @return false when the connection must close */
bool development_app_handle(DevelopmentRequestContext& context, const DevelopmentFrame& frame);
