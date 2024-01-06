#include "tactility.h"

#include "app_manifest_registry.h"
#include "devices_i.h"
#include "furi.h"
#include "graphics_i.h"
#include "partitions.h"
#include "service_registry.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define TAG "tactility"

// System services
extern const ServiceManifest gui_service;
extern const ServiceManifest loader_service;
extern const ServiceManifest desktop_service;
extern const ServiceManifest wifi_service;

// System apps
extern const AppManifest system_info_app;
extern const AppManifest wifi_app;

int32_t wifi_main(void* p);

static void register_system_apps() {
    FURI_LOG_I(TAG, "Registering default apps");
    app_manifest_registry_add(&system_info_app);
    app_manifest_registry_add(&wifi_app);
}

static void register_user_apps(const Config* _Nonnull config) {
    FURI_LOG_I(TAG, "Registering user apps");
    for (size_t i = 0; i < CONFIG_APPS_LIMIT; i++) {
        const AppManifest* manifest = config->apps[i];
        if (manifest != NULL) {
            app_manifest_registry_add(manifest);
        } else {
            // reached end of list
            break;
        }
    }
}

static void register_system_services() {
    FURI_LOG_I(TAG, "Registering system services");
    service_registry_add(&gui_service);
    service_registry_add(&loader_service);
    service_registry_add(&desktop_service);
    service_registry_add(&wifi_service);
}

static void start_system_services() {
    FURI_LOG_I(TAG, "Starting system services");
    service_registry_start(gui_service.id);
    service_registry_start(loader_service.id);
    service_registry_start(desktop_service.id);
    service_registry_start(wifi_service.id);
}

static void register_and_start_user_services(const Config* _Nonnull config) {
    FURI_LOG_I(TAG, "Registering and starting user services");
    for (size_t i = 0; i < CONFIG_SERVICES_LIMIT; i++) {
        const ServiceManifest* manifest = config->services[i];
        if (manifest != NULL) {
            service_registry_add(manifest);
            service_registry_start(manifest->id);
        } else {
            // reached end of list
            break;
        }
    }
}

__attribute__((unused)) extern void tactility_start(const Config* _Nonnull config) {

    furi_init();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    tt_partitions_init();

    Hardware hardware = tt_hardware_init(config->hardware);
    /*NbLvgl lvgl =*/tt_graphics_init(&hardware);

    // Register all apps
    register_system_services();
    register_system_apps();
    // TODO: move this after start_system_services, but desktop must subscribe to app registry events first.
    register_user_apps(config);

    // Start all services
    start_system_services();
    register_and_start_user_services(config);

    // Network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_main(NULL);
}
