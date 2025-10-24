#ifdef ESP_PLATFORM

#include <Tactility/network/HttpServer.h>
#include <Tactility/service/wifi/Wifi.h>

namespace tt::network {

constexpr auto* TAG = "HttpServer";

bool HttpServer::startInternal() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = stackSize;
    config.server_port = port;
    config.uri_match_fn = matchUri;

    if (httpd_start(&server, &config) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to start http server on port %lu", port);
        return false;
    }

    for (std::vector<httpd_uri_t>::reference handler : handlers) {
        httpd_register_uri_handler(server, &handler);
    }

    TT_LOG_I(TAG, "Started on port %d", config.server_port);

    return true;
}

void HttpServer::stopInternal() {
    TT_LOG_I(TAG, "Stopping server");
    if (server != nullptr && httpd_stop(server) != ESP_OK) {
        TT_LOG_W(TAG, "Error while stopping");
        server = nullptr;
    }
}

void HttpServer::start() {
    auto lock = mutex.asScopedLock();
    lock.lock();

    startInternal();
}

void HttpServer::stop() {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (!isStarted()) {
        TT_LOG_W(TAG, "Not started");
    }

    stopInternal();
}

}

#endif