#pragma once

#include <Tactility/PubSub.h>
#include <Tactility/service/Service.h>
#include <Tactility/RecursiveMutex.h>

#include <http/server.h>

#ifdef ESP_PLATFORM
#include <esp_netif.h>
#endif

#include <string>

namespace tt::service::webserver {

enum class WebServerEvent {
    /** WebServer settings have been modified (WiFi/HTTP credentials, enable/disable states) */
    WebServerSettingsChanged,
    /** HTTP server has started and is accepting connections */
    WebServerStarted,
    /** HTTP server has stopped and is no longer accepting connections */
    WebServerStopped
};

/**
 * @brief Web server service with resilient asset architecture
 *
 * Provides:
 * - Core HTML endpoints (hardcoded in firmware)
 * - Dynamic asset serving from Data partition
 * - SD card fallback
 * - Asset synchronization
 */
class WebServerService final : public Service {
private:
    mutable RecursiveMutex mutex;
    struct HttpServer* httpServer = nullptr;
    PubSub<WebServerEvent>::SubscriptionHandle settingsEventSubscription = nullptr;
    std::shared_ptr<PubSub<WebServerEvent>> pubsub = std::make_shared<PubSub<WebServerEvent>>();
    int8_t statusbarIconId = -1;  // Statusbar icon for WebServer state

    // AP mode WiFi management - real hardware only, see startApMode()/stopApMode()'s own
    // non-ESP_PLATFORM definitions.
    bool apWifiInitialized = false;
#ifdef ESP_PLATFORM
    esp_netif_t* apNetif = nullptr;
#endif

    bool startApMode();
    void stopApMode();

    // Core HTML endpoints (hardcoded in firmware)
    static error_t handleRoot(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleSync(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleReboot(struct HttpServerRequest* request, void* user_ctx);

    // File browser endpoints
    static error_t handleFileBrowser(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleFsList(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleFsTree(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleFsDownload(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleFsMkdir(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleFsDelete(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleFsRename(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleFsUpload(struct HttpServerRequest* request, void* user_ctx);
    // Consolidated dispatch handlers to reduce URI handler table usage
    static error_t handleFsGenericGet(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleFsGenericPost(struct HttpServerRequest* request, void* user_ctx);
    // Admin dispatcher to consolidate small POST endpoints (sync/reboot)
    static error_t handleAdminPost(struct HttpServerRequest* request, void* user_ctx);

    // API endpoints
    static error_t handleApiGet(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleApiPost(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleApiPut(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleApiSysinfo(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleApiApps(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleApiAppsRun(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleApiAppsUninstall(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleApiAppsInstall(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleApiWifi(struct HttpServerRequest* request, void* user_ctx);
    static error_t handleApiScreenshot(struct HttpServerRequest* request, void* user_ctx);

    // Dynamic asset serving
    static error_t handleAssets(struct HttpServerRequest* request, void* user_ctx);

    bool startServer();
    void stopServer();

public:

    ~WebServerService() override;

    bool onStart(ServiceContext& service) override;
    void onStop(ServiceContext& service) override;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    std::shared_ptr<PubSub<WebServerEvent>> getPubsub() const { return pubsub; }
};

// Global accessor for controlling the WebServer service
void setWebServerEnabled(bool enabled);

// Returns whether the HTTP server is actually running right now (not just the persisted setting)
bool isWebServerEnabled();

// Get the pubsub for subscribing to WebServer events
std::shared_ptr<PubSub<WebServerEvent>> getPubsub();

} // namespace
