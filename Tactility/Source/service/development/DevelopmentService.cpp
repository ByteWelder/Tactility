#ifdef ESP_PLATFORM

#include "Tactility/service/development/DevelopmentService.h"

#include "Tactility/network/Url.h"
#include "Tactility/TactilityHeadless.h"
#include "Tactility/service/ServiceManifest.h"
#include "Tactility/service/ServiceRegistry.h"
#include "Tactility/service/wifi/Wifi.h"

#include <cstring>
#include <esp_wifi.h>
#include <sstream>
#include <Tactility/Preferences.h>
#include <Tactility/app/App.h>
#include <Tactility/app/ManifestRegistry.h>

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
        return nullptr;
    }
    while (currentLength < contentLength) {
        received = httpd_req_recv(request, buffer + currentLength, contentLength);
        if (received <= 0) {
            // Respond with 500 Internal Server Error
            httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return nullptr;
        }
        currentLength += received;
    }
    buffer[contentLength] = '\0';
    return buffer;
}

void DevelopmentService::onStart(ServiceContext& service) {
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

    setEnabled(isEnabledOnStart());
}

void DevelopmentService::onStop(ServiceContext& service) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    kernel::unsubscribeSystemEvent(networkConnectEventSubscription);
    kernel::unsubscribeSystemEvent(networkDisconnectEventSubscription);

    if (isEnabled()) {
        setEnabled(false);
    }
}

// region Enable/disable

void DevelopmentService::setEnabled(bool enabled) {
    auto lock = mutex.asScopedLock();
    lock.lock();
    this->enabled = enabled;

    // We might already have an IP address, so in case we do, we start the server manually
    // Or we started the server while it shouldn't be
    if (enabled && !isStarted() && wifi::getRadioState() == wifi::RadioState::ConnectionActive) {
        startServer();
    } else if (!enabled && isStarted()) {
        stopServer();
    }
}

bool DevelopmentService::isEnabled() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return enabled;
}

bool DevelopmentService::isEnabledOnStart() const {
    Preferences preferences = Preferences(manifest.id.c_str());
    bool enabled_on_boot = false;
    preferences.optBool("enabledOnBoot", enabled_on_boot);
    return enabled_on_boot;
}

void DevelopmentService::setEnabledOnStart(bool enabled) {
    Preferences preferences = Preferences(manifest.id.c_str());
    preferences.putBool("enabledOnBoot", enabled);
}

// region Enable/disable

void DevelopmentService::startServer() {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (isStarted()) {
        TT_LOG_W(TAG, "Already started");
        return;
    }

    ESP_LOGI(TAG, "Starting server");

    std::stringstream stream;
    stream << "{";
    stream << "\"cpuFamily\":\"" << CONFIG_IDF_TARGET << "\", ";
    stream << "\"osVersion\":\"" << TT_VERSION << "\", ";
    stream << "\"protocolVersion\":\"1.0.0\"";
    stream << "}";
    deviceResponse = stream.str();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = 6666;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &handleGetInfoEndpoint);
        httpd_register_uri_handler(server, &appRunEndpoint);
        httpd_register_uri_handler(server, &appInstallEndpoint);
        TT_LOG_I(TAG, "Started on port %d", config.server_port);
    } else {
        TT_LOG_E(TAG, "Failed to start");
    }
}

void DevelopmentService::stopServer() {
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

bool DevelopmentService::isStarted() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return server != nullptr;
}

void DevelopmentService::onNetworkConnected() {
    TT_LOG_I(TAG, "onNetworkConnected");
    mutex.withLock([this] {
        if (isEnabled() && !isStarted()) {
            startServer();
        }
    });
}

void DevelopmentService::onNetworkDisconnected() {
    TT_LOG_I(TAG, "onNetworkDisconnected");
    mutex.withLock([this] {
        if (isStarted()) {
            stopServer();
        }
    });
}

// region endpoints

esp_err_t DevelopmentService::handleGetInfo(httpd_req_t* request) {
    if (httpd_resp_set_type(request, "application/json") != ESP_OK) {
        TT_LOG_W(TAG, "Failed to send header");
        return ESP_FAIL;
    }

    auto* service = static_cast<DevelopmentService*>(request->user_ctx);

    if (httpd_resp_sendstr(request, service->deviceResponse.c_str()) != ESP_OK) {
        TT_LOG_W(TAG, "Failed to send response body");
        return ESP_FAIL;
    }

    TT_LOG_I(TAG, "[200] /device");
    return ESP_OK;
}

esp_err_t DevelopmentService::handleAppRun(httpd_req_t* request) {
    size_t buffer_length = httpd_req_get_url_query_len(request);
    if (buffer_length == 0) {
        TT_LOG_W(TAG, "[400] /app/run id not specified");
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "id not specified");
        return ESP_FAIL;
    }

    auto buffer = std::make_unique<char[]>(buffer_length + 1);
    if (buffer.get() == nullptr || httpd_req_get_url_query_str(request, buffer.get(), buffer_length + 1) != ESP_OK) {
        TT_LOG_W(TAG, "[500] /app/run");
        httpd_resp_send_500(request);
        return ESP_FAIL;
    }

    auto parameters = network::parseUrlQuery(std::string(buffer.get()));
    auto id_key_pos = parameters.find("id");
    if (id_key_pos == parameters.end()) {
        TT_LOG_W(TAG, "[400] /app/run id not specified");
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "id not specified");
        return ESP_FAIL;
    }

    auto app_id = id_key_pos->second;
    if (!app::findAppById(app_id.c_str())) {
        TT_LOG_W(TAG, "[400] /app/run app not found");
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "app not found");
        return ESP_FAIL;
    }

    app::start(app_id);
    TT_LOG_I(TAG, "[200] /app/run %s", app_id.c_str());
    httpd_resp_send(request, nullptr, 0);
    return ESP_OK;
}

esp_err_t DevelopmentService::handleAppInstall(httpd_req_t* request) {
    size_t header_size = httpd_req_get_hdr_value_len(request, "Content-Type");
    if (header_size == 0) {
        TT_LOG_W(TAG, "[400] /app/install Content-Type header not specified");
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Content-Type header not specified");
        return ESP_FAIL;
    }

    auto header_buffer = std::make_unique<char[]>(header_size + 1);
    if (header_buffer.get() == nullptr || httpd_req_get_hdr_value_str(request, "Content-Type", header_buffer.get(), header_size + 1) != ESP_OK) {
        TT_LOG_W(TAG, "[500] /app/run");
        httpd_resp_send_500(request);
        return ESP_FAIL;
    }

    TT_LOG_I(TAG, "[200] /app/install %s", header_buffer.get());
    httpd_resp_send(request, nullptr, 0);
    return ESP_OK;
}

// endregion

std::shared_ptr<DevelopmentService> findService() {
    return std::static_pointer_cast<DevelopmentService>(
        findServiceById(manifest.id)
    );
}

extern const ServiceManifest manifest = {
    .id = "Development",
    .createService = create<DevelopmentService>
};

}

#endif // ESP_PLATFORM
