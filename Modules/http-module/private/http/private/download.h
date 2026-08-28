// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <http/download.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/time.h>

#include <cstdio>
#include <string>

// Shared between a subscription and its download task, so http_download_unsubscribe() can sever
// the link at any time without racing the task's access to the subscription/event_group.
// Refcounted via `owners`: starts at 1 for the subscriber, gains 1 when http_download_start()
// spawns the task, and whichever side releases last deletes this.
// `mutex` also guards the subscription's `internal.event`/`pending`/`event_group`/`bit`. The task
// only touches those while `subscribed` is true, under this same lock, so that check is a hard
// guarantee those fields are still live.
struct HttpDownloadLink {
    Mutex mutex {};
    bool subscribed = true;
    bool cancel_requested = false;
    int owners = 1;

    HttpDownloadLink() { mutex_construct(&mutex); }
    ~HttpDownloadLink() { mutex_destruct(&mutex); }
};

inline HttpDownloadEvent http_download_make_error_event(const char* message, int32_t status_code = 0) {
    HttpDownloadEvent event {};
    event.type = HTTP_DOWNLOAD_EVENT_ERROR;
    event.timestamp = get_micros_since_boot();
    event.status_code = status_code;
    snprintf(event.error.message, sizeof(event.error.message), "%s", message);
    return event;
}

inline HttpDownloadEvent http_download_make_success_event(int32_t status_code) {
    HttpDownloadEvent event {};
    event.type = HTTP_DOWNLOAD_EVENT_SUCCESS;
    event.timestamp = get_micros_since_boot();
    event.status_code = status_code;
    return event;
}

inline HttpDownloadEvent http_download_make_cancelled_event(int32_t status_code = 0) {
    HttpDownloadEvent event {};
    event.type = HTTP_DOWNLOAD_EVENT_CANCELLED;
    event.timestamp = get_micros_since_boot();
    event.status_code = status_code;
    return event;
}

inline bool http_download_is_cancelled(HttpDownloadLink* link) {
    mutex_lock(&link->mutex);
    bool cancelled = link->cancel_requested;
    mutex_unlock(&link->mutex);
    return cancelled;
}

/** Implemented in download_esp.cpp (ESP_PLATFORM) or download_mock.cpp (otherwise). */
HttpDownloadEvent http_download_run(
    const std::string& url,
    const std::string& certPath,
    const std::string& targetPath,
    HttpDownloadLink* link
);
