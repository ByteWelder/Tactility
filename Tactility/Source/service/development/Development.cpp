#ifdef ESP_PLATFORM

#include "Tactility/service/development/Development.h"

#include "Tactility/TactilityHeadless.h"
#include "Tactility/service/ServiceManifest.h"
#include "Tactility/service/ServiceRegistry.h"

#include <cstring>
#include <esp_wifi.h>
#include <sstream>

namespace tt::service::development {

extern const ServiceManifest manifest;

constexpr const char* TAG = "DevService";


static char* rest_read_buffer(httpd_req_t* request) {
    static char buffer[1024];
    int contentLength = request->content_len;
    int currentLength = 0;
    int received = 0;
    if (contentLength >= 1024) {
        // Respond with 500 Internal Server Error
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return NULL;
    }
    while (currentLength < contentLength) {
        received = httpd_req_recv(request, buffer + currentLength, contentLength);
        if (received <= 0) {
            // Respond with 500 Internal Server Error
            httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return NULL;
        }
        currentLength += received;
    }
    buffer[contentLength] = '\0';
    return buffer;
}

void Development::onStart(ServiceContext& service) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    networkConnectEventSubscription = kernel::subscribeSystemEvent(
        kernel::SystemEvent::NetworkConnected,
        [this](kernel::SystemEvent) { onNetworkConnected(); }
    );
    networkConnectEventSubscription = kernel::subscribeSystemEvent(
        kernel::SystemEvent::NetworkDisconnected,
        [this](kernel::SystemEvent) { onNetworkDisconnected(); }
    );

    setEnabled(true);
}

void Development::onStop(ServiceContext& service) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    kernel::unsubscribeSystemEvent(networkConnectEventSubscription);
    kernel::unsubscribeSystemEvent(networkDisconnectEventSubscription);

    if (isEnabled()) {
        setEnabled(false);
    }
}

// region Enable/disable

void Development::setEnabled(bool enabled) {
    auto lock = mutex.asScopedLock();
    lock.lock();
    this->enabled = enabled;
}

bool Development::isEnabled() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return enabled;
}

// region Enable/disable

// region Handlers

void Development::startServer() {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (isStarted()) {
        TT_LOG_W(TAG, "Already started");
        return;
    }

    ESP_LOGI(TAG, "Starting server");

    std::stringstream stream;
    stream << "{";
    stream << "\"cpuFamily\" : \"" << CONFIG_IDF_TARGET << "\"";
    stream << "}";
    deviceResponse = stream.str();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = 6666;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &getDeviceEndpoint);
        TT_LOG_I(TAG, "Started on port %d", config.server_port);
    } else {
        TT_LOG_E(TAG, "Failed to start");
    }
}

void Development::stopServer() {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (!isStarted()) {
        TT_LOG_W(TAG, "Not started");
        return;
    }

    TT_LOG_I(TAG, "Stopping server");
    if (httpd_stop(server) != ESP_OK) {
        TT_LOG_W(TAG, "Error while stopping");
    }
    server = nullptr;
}

bool Development::isStarted() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return server != nullptr;
}

void Development::onNetworkConnected() {
    TT_LOG_I(TAG, "onNetworkConnected");
    mutex.withLock([this] {
        if (isEnabled() && !isStarted()) {
            startServer();
        }
    });
}

void Development::onNetworkDisconnected() {
    TT_LOG_I(TAG, "onNetworkDisconnected");
    mutex.withLock([this] {
        if (isStarted()) {
            stopServer();
        }
    });
}

// endregion

// region endpoints

esp_err_t Development::getDevice(httpd_req_t* request) {
    if (httpd_resp_set_type(request, "application/json") != ESP_OK) {
        TT_LOG_W(TAG, "Failed to send header");
        return ESP_FAIL;
    }

    auto* service = static_cast<Development*>(request->user_ctx);

    if (httpd_resp_sendstr(request, service->deviceResponse.c_str()) != ESP_OK) {
        TT_LOG_W(TAG, "Failed to send response body");
        return ESP_FAIL;
    }

    TT_LOG_I(TAG, "[200] /device from");
    return ESP_OK;
}

std::shared_ptr<Development> findService() {
    return std::static_pointer_cast<Development>(
        findServiceById(manifest.id)
    );
}

extern const ServiceManifest manifest = {
    .id = "Development",
    .createService = create<Development>
};

}

#endif // ESP_PLATFORM
