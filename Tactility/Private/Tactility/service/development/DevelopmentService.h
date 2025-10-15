#pragma once
#ifdef ESP_PLATFORM

#include <Tactility/service/Service.h>

#include <Tactility/Mutex.h>

#include <esp_event.h>
#include <esp_http_server.h>
#include <Tactility/network/HttpServer.h>

namespace tt::service::development {

class DevelopmentService final : public Service {

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::string deviceResponse;
    network::HttpServer httpServer = network::HttpServer(
        6666,
        "0.0.0.0",
        std::vector<httpd_uri_t>{
            {
                .uri = "/info",
                .method = HTTP_GET,
                .handler = handleGetInfo,
                .user_ctx = this
            },
            {
                .uri = "/app/run",
                .method = HTTP_POST,
                .handler = handleAppRun,
                .user_ctx = this
            },
            {
                .uri = "/app/install",
                .method = HTTP_PUT,
                .handler = handleAppInstall,
                .user_ctx = this
            },
            {
                .uri = "/app/uninstall",
                .method = HTTP_PUT,
                .handler = handleAppUninstall,
                .user_ctx = this
            }
        }
    );

    void startServer();
    void stopServer();

    static esp_err_t handleGetInfo(httpd_req_t* request);
    static esp_err_t handleAppRun(httpd_req_t* request);
    static esp_err_t handleAppInstall(httpd_req_t* request);
    static esp_err_t handleAppUninstall(httpd_req_t* request);

public:

    // region Overrides

    bool onStart(ServiceContext& service) override;

    void onStop(ServiceContext& service) override;

    // endregion Overrides

    // region Internal API

    /**
     * Enabling the service means that the user is willing to start the web server.
     * @return true when the service is enabled
     */
    bool isEnabled() const;

    /**
     * Enabling the service means that the user is willing to start the web server.
     * @param[in] enabled
     */
    void setEnabled(bool enabled);
};

std::shared_ptr<DevelopmentService> findService();

}

#endif // ESP_PLATFORM
