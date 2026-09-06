#pragma once

#include <Tactility/service/Service.h>

#include <Tactility/RecursiveMutex.h>

#include <http/server.h>

namespace tt::service::development {

class DevelopmentService final : public Service {

    RecursiveMutex mutex;
    std::string deviceResponse;
    struct HttpServer* httpServer = nullptr;

    void startServer();
    void stopServer();

    static error_t handleGetInfo(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleAppRun(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleAppInstall(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleAppUninstall(struct HttpServerRequest* request, void* user_ctx);

public:

    DevelopmentService();
    ~DevelopmentService() override;

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
