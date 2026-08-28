// SPDX-License-Identifier: Apache-2.0
#include <http/download.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/freertos/task.h>
#include <tactility/log.h>
#include <tactility/time.h>

#include <cstdio>
#include <new>
#include <string>

#ifdef ESP_PLATFORM
#include <esp_http_client.h>
#endif

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

namespace {

constexpr auto* TAG = "http-download";
constexpr size_t DOWNLOAD_TASK_STACK_DEPTH = 4608 / sizeof(StackType_t);

struct DownloadContext {
    std::string url;
    std::string certPath;
    std::string targetPath;
    HttpDownloadSubscription* subscription;
    HttpDownloadLink* link;
};

HttpDownloadEvent make_error_event(const char* message, int32_t status_code = 0) {
    HttpDownloadEvent event {};
    event.type = HTTP_DOWNLOAD_EVENT_ERROR;
    event.timestamp = get_micros_since_boot();
    event.status_code = status_code;
    snprintf(event.error.message, sizeof(event.error.message), "%s", message);
    return event;
}

HttpDownloadEvent make_success_event(int32_t status_code) {
    HttpDownloadEvent event {};
    event.type = HTTP_DOWNLOAD_EVENT_SUCCESS;
    event.timestamp = get_micros_since_boot();
    event.status_code = status_code;
    return event;
}

HttpDownloadEvent make_cancelled_event(int32_t status_code = 0) {
    HttpDownloadEvent event {};
    event.type = HTTP_DOWNLOAD_EVENT_CANCELLED;
    event.timestamp = get_micros_since_boot();
    event.status_code = status_code;
    return event;
}

bool is_cancelled(HttpDownloadLink* link) {
    mutex_lock(&link->mutex);
    bool cancelled = link->cancel_requested;
    mutex_unlock(&link->mutex);
    return cancelled;
}

#ifdef ESP_PLATFORM

// RAII: guarantees esp_http_client_close()/_cleanup() run on every exit path below.
class EspDownloadClient {
    esp_http_client_handle_t client = nullptr;
    bool isOpen = false;

public:
    ~EspDownloadClient() {
        if (isOpen) {
            esp_http_client_close(client);
        }
        if (client != nullptr) {
            esp_http_client_cleanup(client);
        }
    }

    bool init(const esp_http_client_config_t& config) {
        client = esp_http_client_init(&config);
        return client != nullptr;
    }

    bool open() {
        if (esp_http_client_open(client, 0) != ESP_OK) {
            return false;
        }
        isOpen = true;
        return true;
    }

    bool fetchHeaders() const { return esp_http_client_fetch_headers(client) >= 0; }

    int getStatusCode() const { return esp_http_client_get_status_code(client); }

    int getContentLength() const { return esp_http_client_get_content_length(client); }

    int read(char* buffer, int size) const { return esp_http_client_read(client, buffer, size); }
};

// esp_http_client_config_t::cert_pem needs a NUL-terminated buffer.
bool read_certificate(const std::string& certPath, std::string& outCertificate) {
    auto* file = fopen(certPath.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size <= 0) {
        fclose(file);
        return false;
    }

    outCertificate.resize(static_cast<size_t>(size));
    size_t read_bytes = fread(outCertificate.data(), 1, static_cast<size_t>(size), file);
    fclose(file);
    return read_bytes == static_cast<size_t>(size);
}

HttpDownloadEvent run_download(const std::string& url, const std::string& certPath, const std::string& targetPath, HttpDownloadLink* link) {
    if (is_cancelled(link)) {
        return make_cancelled_event();
    }

    std::string certificate;
    if (!read_certificate(certPath, certificate)) {
        return make_error_event("Failed to read certificate file");
    }

    esp_http_client_config_t config {};
    config.url = url.c_str();
    config.auth_type = HTTP_AUTH_TYPE_NONE;
    config.cert_pem = certificate.c_str();
    config.cert_len = certificate.size() + 1;
    config.tls_version = ESP_HTTP_CLIENT_TLS_VER_TLS_1_2;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 5000;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;

    EspDownloadClient client;
    if (!client.init(config)) {
        return make_error_event("Failed to initialize HTTP client");
    }
    if (!client.open()) {
        return make_error_event("Failed to open connection");
    }
    if (!client.fetchHeaders()) {
        return make_error_event("Failed to fetch response headers");
    }

    auto status_code = client.getStatusCode();
    if (status_code < 200 || status_code >= 300) {
        return make_error_event("Server response is not OK", status_code);
    }

    if (is_cancelled(link)) {
        return make_cancelled_event(status_code);
    }

    // Downloaded to a sibling ".tmp" file first and only renamed onto targetPath on success, so a
    // failed/cancelled download (or a crash mid-write) never leaves a partial file at targetPath.
    auto tempPath = targetPath + ".tmp";
    auto* file = fopen(tempPath.c_str(), "wb");
    if (file == nullptr) {
        return make_error_event("Failed to open target file", status_code);
    }

    auto bytes_left = client.getContentLength();
    char buffer[512];
    while (bytes_left > 0) {
        if (is_cancelled(link)) {
            fclose(file);
            remove(tempPath.c_str());
            return make_cancelled_event(status_code);
        }
        int data_read = client.read(buffer, sizeof(buffer));
        if (data_read <= 0) {
            fclose(file);
            remove(tempPath.c_str());
            return make_error_event("Failed to read response data", status_code);
        }
        bytes_left -= data_read;
        if (fwrite(buffer, 1, static_cast<size_t>(data_read), file) != static_cast<size_t>(data_read)) {
            fclose(file);
            remove(tempPath.c_str());
            return make_error_event("Failed to write downloaded data", status_code);
        }
        taskYIELD();
    }
    fclose(file);

    // Some embedded filesystems (e.g. FATFS) reject rename() onto an existing path instead of
    // replacing it like POSIX does. Clear the way first.
    remove(targetPath.c_str());
    if (rename(tempPath.c_str(), targetPath.c_str()) != 0) {
        remove(tempPath.c_str());
        return make_error_event("Failed to finalize downloaded file", status_code);
    }

    return make_success_event(status_code);
}

#else

HttpDownloadEvent run_download(const std::string&, const std::string&, const std::string&, HttpDownloadLink* link) {
    if (is_cancelled(link)) {
        return make_cancelled_event();
    }
    return make_error_event("HTTP downloads are not supported on this platform");
}

#endif

// Delivers `event` to `sub` if the subscriber hasn't unsubscribed in the meantime, then releases
// this task's share of `link`, deleting it if the subscriber already released theirs.
void finish(HttpDownloadLink* link, HttpDownloadSubscription* sub, const HttpDownloadEvent& event) {
    mutex_lock(&link->mutex);
    if (link->subscribed) {
        sub->internal.event = event;
        sub->internal.pending = true;
        task_event_group_signal(sub->internal.event_group, sub->bit);
    }
    bool last = (--link->owners == 0);
    mutex_unlock(&link->mutex);

    if (last) {
        delete link;
    }
}

void download_task_main(void* raw_context) {
    auto* context = static_cast<DownloadContext*>(raw_context);
    LOG_I(TAG, "Downloading %s to %s", context->url.c_str(), context->targetPath.c_str());

    auto event = run_download(context->url, context->certPath, context->targetPath, context->link);
    if (event.type == HTTP_DOWNLOAD_EVENT_ERROR) {
        LOG_E(TAG, "Download of %s failed: %s", context->url.c_str(), event.error.message);
    } else if (event.type == HTTP_DOWNLOAD_EVENT_CANCELLED) {
        LOG_I(TAG, "Download of %s cancelled", context->url.c_str());
    } else {
        LOG_I(TAG, "Downloaded %s to %s", context->url.c_str(), context->targetPath.c_str());
    }

    finish(context->link, context->subscription, event);
    delete context;
    vTaskDelete(nullptr);
}

}

extern "C" {

error_t http_download_subscribe(HttpDownloadSubscription* sub, TaskEventGroup* event_group) {
    uint32_t bit;
    error_t claim_result = task_event_group_claim_bit(event_group, &bit);
    if (claim_result != ERROR_NONE) {
        return claim_result;
    }

    auto* link = new (std::nothrow) HttpDownloadLink();
    if (link == nullptr) {
        task_event_group_release_bit(event_group, bit);
        return ERROR_OUT_OF_MEMORY;
    }

    sub->bit = bit;
    sub->internal.event_group = event_group;
    sub->internal.pending = false;
    sub->internal.link = link;

    return ERROR_NONE;
}

error_t http_download_unsubscribe(HttpDownloadSubscription* sub) {
    HttpDownloadLink* link = sub->internal.link;
    if (link == nullptr) {
        return ERROR_NOT_FOUND;
    }
    // Caller-exclusive: the task never reads this field, so it's safe to clear without the lock.
    sub->internal.link = nullptr;

    TaskEventGroup* event_group;
    uint32_t bit;

    mutex_lock(&link->mutex);
    // Read/clear under the lock: finish() reads these same fields under that lock too,
    // while `subscribed` is true, so this must not race it.
    event_group = sub->internal.event_group;
    bit = sub->bit;
    sub->internal.event_group = nullptr;
    link->subscribed = false;
    bool last = (--link->owners == 0);
    mutex_unlock(&link->mutex);

    // Safe outside the lock: the critical section above already decided whether finish() will
    // ever signal this bit, so nothing will touch it again from here on.
    task_event_group_release_bit(event_group, bit);

    if (last) {
        delete link;
    }

    return ERROR_NONE;
}

error_t http_download_poll(HttpDownloadSubscription* sub, HttpDownloadEvent* out_event) {
    HttpDownloadLink* link = sub->internal.link;
    if (link == nullptr) {
        return ERROR_TIMEOUT;
    }

    mutex_lock(&link->mutex);
    bool pending = sub->internal.pending;
    if (pending) {
        *out_event = sub->internal.event;
        sub->internal.pending = false;
    }
    mutex_unlock(&link->mutex);

    return pending ? ERROR_NONE : ERROR_TIMEOUT;
}

error_t http_download_start(
    const char* url,
    const char* cert_path,
    const char* target_path,
    HttpDownloadSubscription* sub
) {
    HttpDownloadLink* link = sub->internal.link;
    if (link == nullptr) {
        return ERROR_INVALID_ARGUMENT;
    }

    auto* context = new(std::nothrow) DownloadContext {
        .url = url,
        .certPath = cert_path,
        .targetPath = target_path,
        .subscription = sub,
        .link = link
    };
    if (context == nullptr) {
        return ERROR_OUT_OF_MEMORY;
    }

    mutex_lock(&link->mutex);
    link->owners++; // the task's own share, released by finish()
    mutex_unlock(&link->mutex);

    TaskHandle_t task_handle;
    if (xTaskCreate(download_task_main, "http-download", DOWNLOAD_TASK_STACK_DEPTH, context, tskIDLE_PRIORITY + 1, &task_handle) != pdPASS) {
        delete context;

        mutex_lock(&link->mutex);
        bool last = (--link->owners == 0);
        mutex_unlock(&link->mutex);
        if (last) {
            delete link;
        }

        return ERROR_OUT_OF_MEMORY;
    }

    return ERROR_NONE;
}

error_t http_download_cancel(HttpDownloadSubscription* sub) {
    HttpDownloadLink* link = sub->internal.link;
    if (link == nullptr) {
        return ERROR_INVALID_STATE;
    }

    mutex_lock(&link->mutex);
    link->cancel_requested = true;
    mutex_unlock(&link->mutex);

    return ERROR_NONE;
}

}
