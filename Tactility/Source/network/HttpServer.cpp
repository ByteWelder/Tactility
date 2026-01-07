#ifdef ESP_PLATFORM

#include <Tactility/network/HttpServer.h>

#include <Tactility/Logger.h>
#include <Tactility/service/wifi/Wifi.h>

namespace tt::network {

static const auto LOGGER = Logger("HttpServer");

bool HttpServer::startInternal() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = stackSize;
    config.server_port = port;
    config.uri_match_fn = matchUri;

    if (httpd_start(&server, &config) != ESP_OK) {
        LOGGER.error("Failed to start http server on port {}", port);
        return false;
    }

    for (std::vector<httpd_uri_t>::reference handler : handlers) {
        httpd_register_uri_handler(server, &handler);
    }

    LOGGER.info("Started on port {}", config.server_port);

    return true;
}

void HttpServer::stopInternal() {
    LOGGER.info("Stopping server");
    if (server != nullptr && httpd_stop(server) != ESP_OK) {
        LOGGER.warn("Error while stopping");
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
        LOGGER.warn("Not started");
    }

    stopInternal();
}

}

#endif