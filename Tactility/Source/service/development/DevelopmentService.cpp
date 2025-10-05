#ifdef ESP_PLATFORM

#include <Tactility/service/development/DevelopmentService.h>

#include <Tactility/app/App.h>
#include <Tactility/app/AppRegistration.h>
#include <Tactility/file/File.h>
#include <Tactility/network/HttpdReq.h>
#include <Tactility/network/Url.h>
#include <Tactility/Paths.h>
#include <Tactility/service/development/DevelopmentSettings.h>
#include <Tactility/service/ServiceManifest.h>
#include <Tactility/service/ServiceRegistration.h>
#include <Tactility/service/wifi/Wifi.h>
#include <Tactility/StringUtils.h>

#include <cstring>
#include <esp_wifi.h>
#include <ranges>
#include <sstream>
#include <Tactility/Tactility.h>
#include <Tactility/file/FileLock.h>

namespace tt::service::development {

extern const ServiceManifest manifest;

constexpr const char* TAG = "DevService";

bool DevelopmentService::onStart(ServiceContext& service) {
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

    setEnabled(shouldEnableOnBoot());

    return true;
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
    config.stack_size = 5120;

    config.server_port = 6666;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &handleGetInfoEndpoint);
        httpd_register_uri_handler(server, &appRunEndpoint);
        httpd_register_uri_handler(server, &appInstallEndpoint);
        httpd_register_uri_handler(server, &appUninstallEndpoint);
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
    TT_LOG_I(TAG, "GET /device");

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
    TT_LOG_I(TAG, "POST /app/run");

    std::string query;
    if (!network::getQueryOrSendError(request, query)) {
        return ESP_FAIL;
    }

    auto parameters = network::parseUrlQuery(query);
    auto id_key_pos = parameters.find("id");
    if (id_key_pos == parameters.end()) {
        TT_LOG_W(TAG, "[400] /app/run id not specified");
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "id not specified");
        return ESP_FAIL;
    }

    const auto& app_id = id_key_pos->second;
    if (app::isRunning(app_id)) {
        app::stopAll(app_id);
    }

    app::start(app_id);

    TT_LOG_I(TAG, "[200] /app/run %s", id_key_pos->second.c_str());
    httpd_resp_send(request, nullptr, 0);

    return ESP_OK;
}

esp_err_t DevelopmentService::handleAppInstall(httpd_req_t* request) {
    TT_LOG_I(TAG, "PUT /app/install");

    std::string boundary;
    if (!network::getMultiPartBoundaryOrSendError(request, boundary)) {
        return false;
    }

    size_t content_left = request->content_len;

    // Skip newline after reading boundary
    auto content_headers_data = network::receiveTextUntil(request, "\r\n\r\n");
    content_left -= content_headers_data.length();
    auto content_headers = string::split(content_headers_data, "\r\n")
        | std::views::filter([](const std::string& line) {
            return line.length() > 0;
        })
        | std::ranges::to<std::vector>();

    auto content_disposition_map = network::parseContentDisposition(content_headers);
    if (content_disposition_map.empty()) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Multipart form error: invalid content disposition");
        return ESP_FAIL;
    }

    auto name_entry = content_disposition_map.find("name");
    auto filename_entry = content_disposition_map.find("filename");
    if (
        name_entry == content_disposition_map.end() ||
        filename_entry == content_disposition_map.end() ||
        name_entry->second != "elf"
    ) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Multipart form error: name or filename parameter missing or mismatching");
        return ESP_FAIL;
    }

    // Receive boundary
    auto boundary_and_newlines_after_file = std::format("\r\n--{}--\r\n", boundary);
    auto file_size = content_left - boundary_and_newlines_after_file.length();

    // Create tmp directory
    const std::string tmp_path = getTempPath();
    if (!file::findOrCreateDirectory(tmp_path, 0777)) {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save file");
        return ESP_FAIL;
    }

    auto file_path = std::format("{}/{}", tmp_path, filename_entry->second);
    if (network::receiveFile(request, file_size, file_path) != file_size) {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save file");
        return ESP_FAIL;
    }

    content_left -= file_size;

    // Read and verify part
    if (!network::readAndDiscardOrSendError(request, boundary_and_newlines_after_file)) {
        return ESP_FAIL;
    }
    content_left -= boundary_and_newlines_after_file.length();

    if (content_left != 0) {
        TT_LOG_W(TAG, "We have more bytes at the end of the request parsing?!");
    }

    if (!app::install(file_path)) {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to install");
        return ESP_FAIL;
    }

    if (!file::deleteFile(file_path.c_str())) {
        TT_LOG_W(TAG, "Failed to delete %s", file_path.c_str());
    }

    TT_LOG_I(TAG, "[200] /app/install -> %s", file_path.c_str());

    httpd_resp_send(request, nullptr, 0);

    return ESP_OK;
}

esp_err_t DevelopmentService::handleAppUninstall(httpd_req_t* request) {
    TT_LOG_I(TAG, "PUT /app/uninstall");

    std::string query;
    if (!network::getQueryOrSendError(request, query)) {
        return ESP_FAIL;
    }

    auto parameters = network::parseUrlQuery(query);
    auto id_key_pos = parameters.find("id");
    if (id_key_pos == parameters.end()) {
        TT_LOG_W(TAG, "[400] /app/uninstall id not specified");
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "id not specified");
        return ESP_FAIL;
    }

    if (!app::findAppById(id_key_pos->second)) {
        TT_LOG_I(TAG, "[200] /app/uninstall %s (app wasn't installed)", id_key_pos->second.c_str());
        httpd_resp_send(request, nullptr, 0);
        return ESP_OK;
    }

    if (app::uninstall(id_key_pos->second)) {
        TT_LOG_I(TAG, "[200] /app/uninstall %s", id_key_pos->second.c_str());
        httpd_resp_send(request, nullptr, 0);
        return ESP_OK;
    } else {
        TT_LOG_W(TAG, "[500] /app/uninstall %s", id_key_pos->second.c_str());
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to uninstall");
        return ESP_FAIL;
    }
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
