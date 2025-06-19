#pragma once
#ifdef ESP_PLATFORM

#include "Tactility/service/Service.h"

#include <Tactility/Mutex.h>

#include <esp_event.h>
#include <esp_http_server.h>
#include <Tactility/kernel/SystemEvents.h>

namespace tt::service::development {

class Development final : public Service {

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    httpd_handle_t server = nullptr;
    bool enabled = false;
    kernel::SystemEventSubscription networkConnectEventSubscription = 0;
    kernel::SystemEventSubscription networkDisconnectEventSubscription = 0;
    std::string deviceResponse;

    httpd_uri_t handleGetInfoEndpoint = {
        .uri = "/info",
        .method = HTTP_GET,
        .handler = handleGetInfo,
        .user_ctx = this
    };

    httpd_uri_t appRunEndpoint = {
        .uri = "/app/run",
        .method = HTTP_POST,
        .handler = handleAppRun,
        .user_ctx = this
    };

    httpd_uri_t appInstallEndpoint = {
        .uri = "/app/install",
        .method = HTTP_POST,
        .handler = handleAppInstall,
        .user_ctx = this
    };

    void onNetworkConnected();
    void onNetworkDisconnected();

    void startServer();
    void stopServer();

    static esp_err_t handleGetInfo(httpd_req_t* request);
    static esp_err_t handleAppRun(httpd_req_t* request);
    static esp_err_t handleAppInstall(httpd_req_t* request);

public:

    // region Overrides

    void onStart(ServiceContext& service) override;
    void onStop(ServiceContext& service) override;

    // endregion Overrides

    // region Internal API

    void setEnabled(bool enabled);

    bool isEnabled() const;

    bool isStarted() const;

    // region Internal API
};

std::shared_ptr<Development> findService();

}

#endif // ESP_PLATFORM
