#ifdef ESP_TARGET

#include "Wifi.h"

#include "MessageQueue.h"
#include "Mutex.h"
#include "Check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "Log.h"
#include "Pubsub.h"
#include "service/Service.h"
#include "WifiSettings.h"
#include <atomic>
#include <cstring>
#include <sys/cdefs.h>
#include <TactilityCore.h>

namespace tt::service::wifi {

#define TAG "wifi_service"
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

typedef enum {
    WifiMessageTypeRadioOn,
    WifiMessageTypeRadioOff,
    WifiMessageTypeScan,
    WifiMessageTypeConnect,
    WifiMessageTypeDisconnect,
    WifiMessageTypeAutoConnect,
} WifiMessageType;

typedef struct {
} WifiConnectMessage;

typedef struct {
    WifiMessageType type;
    union {
        WifiConnectMessage connect_message;
    };
} WifiMessage;

class Wifi {
public:
    Wifi();
    ~Wifi();

    std::atomic<WifiRadioState> radio_state;
    /** @brief Locking mechanism for modifying the Wifi instance */
    Mutex mutex = Mutex(MutexTypeRecursive);
    /** @brief The public event bus */
    PubSub* pubsub = nullptr;
    /** @brief The internal message queue */
    MessageQueue queue = MessageQueue(1, sizeof(WifiMessage));
    // TODO: Deal with messages that come in while an action is ongoing
    // for example: when scanning and you turn off the radio, the scan should probably stop or turning off
    // the radio should disable the on/off button in the app as it is pending.
    /** @brief The network interface when wifi is started */
    esp_netif_t* _Nullable netif = nullptr;
    /** @brief Scanning results */
    wifi_ap_record_t* _Nullable scan_list = nullptr;
    /** @brief The current item count in scan_list (-1 when scan_list is NULL) */
    uint16_t scan_list_count = 0;
    /** @brief Maximum amount of records to scan (value > 0) */
    uint16_t scan_list_limit = TT_WIFI_SCAN_RECORD_LIMIT;
    bool scan_active = false;
    bool secure_connection = false;
    esp_event_handler_instance_t event_handler_any_id = nullptr;
    esp_event_handler_instance_t event_handler_got_ip = nullptr;
    EventFlag connection_wait_flags;
    settings::WifiApSettings connection_target = {
        .ssid = { 0 },
        .password = { 0 },
        .auto_connect = false
    };
    bool connection_target_remember = false; // Whether to store the connection_target on successful connection or not
};

static Wifi* wifi_singleton = nullptr;

// Forward declarations
static void scan_list_free_safely(Wifi* wifi);
static void disconnect_internal_but_keep_active(Wifi* wifi);
static void lock(Wifi* wifi);
static void unlock(Wifi* wifi);

// region Alloc

Wifi::Wifi() : radio_state(WIFI_RADIO_OFF) {
    pubsub = tt_pubsub_alloc();
}

Wifi::~Wifi() {
    tt_pubsub_free(pubsub);
}

// endregion Alloc

// region Public functions

PubSub* getPubsub() {
    tt_assert(wifi_singleton);
    return wifi_singleton->pubsub;
}

WifiRadioState getRadioState() {
    tt_assert(wifi_singleton);
    lock(wifi_singleton);
    WifiRadioState state = wifi_singleton->radio_state;
    unlock(wifi_singleton);
    return state;
}

void scan() {
    tt_assert(wifi_singleton);
    lock(wifi_singleton);
    WifiMessage message = {.type = WifiMessageTypeScan};
    // No need to lock for queue
    wifi_singleton->queue.put(&message, 100 / portTICK_PERIOD_MS);
    unlock(wifi_singleton);
}

bool isScanning() {
    tt_assert(wifi_singleton);
    lock(wifi_singleton);
    bool is_scanning = wifi_singleton->scan_active;
    unlock(wifi_singleton);
    return is_scanning;
}

void connect(const settings::WifiApSettings* ap, bool remember) {
    tt_assert(wifi_singleton);
    lock(wifi_singleton);
    memcpy(&wifi_singleton->connection_target, ap, sizeof(settings::WifiApSettings));
    wifi_singleton->connection_target_remember = remember;
    WifiMessage message = {.type = WifiMessageTypeConnect};
    wifi_singleton->queue.put(&message, 100 / portTICK_PERIOD_MS);
    unlock(wifi_singleton);
}

void disconnect() {
    tt_assert(wifi_singleton);
    lock(wifi_singleton);
    wifi_singleton->connection_target = (settings::WifiApSettings) {
        .ssid = { 0 },
        .password = { 0 },
        .auto_connect = false
    };
    WifiMessage message = {.type = WifiMessageTypeDisconnect};
    wifi_singleton->queue.put(&message, 100 / portTICK_PERIOD_MS);
    unlock(wifi_singleton);
}

void setScanRecords(uint16_t records) {
    tt_assert(wifi_singleton);
    lock(wifi_singleton);
    if (records != wifi_singleton->scan_list_limit) {
        scan_list_free_safely(wifi_singleton);
        wifi_singleton->scan_list_limit = records;
    }
    unlock(wifi_singleton);
}

void getScanResults(WifiApRecord records[], uint16_t limit, uint16_t* result_count) {
    tt_assert(wifi_singleton);
    tt_assert(result_count);

    lock(wifi_singleton);
    if (wifi_singleton->scan_list_count == 0) {
        *result_count = 0;
    } else {
        uint16_t i = 0;
        TT_LOG_I(TAG, "processing up to %d APs", wifi_singleton->scan_list_count);
        uint16_t last_index = TT_MIN(wifi_singleton->scan_list_count, limit);
        for (; i < last_index; ++i) {
            memcpy(records[i].ssid, wifi_singleton->scan_list[i].ssid, 33);
            records[i].rssi = wifi_singleton->scan_list[i].rssi;
            records[i].auth_mode = wifi_singleton->scan_list[i].authmode;
        }
        // The index already overflowed right before the for-loop was terminated,
        // so it effectively became the list count:
        *result_count = i;
    }
    unlock(wifi_singleton);
}

void setEnabled(bool enabled) {
    tt_assert(wifi_singleton);
    lock(wifi_singleton);
    if (enabled) {
        WifiMessage message = {.type = WifiMessageTypeRadioOn};
        // No need to lock for queue
        wifi_singleton->queue.put(&message, 100 / portTICK_PERIOD_MS);
    } else {
        WifiMessage message = {.type = WifiMessageTypeRadioOff};
        // No need to lock for queue
        wifi_singleton->queue.put(&message, 100 / portTICK_PERIOD_MS);
    }
    unlock(wifi_singleton);
}

bool isConnectionSecure() {
    tt_assert(wifi_singleton);
    lock(wifi_singleton);
    bool is_secure = wifi_singleton->secure_connection;
    unlock(wifi_singleton);
    return is_secure;
}

int getRssi() {
    tt_assert(wifi_singleton);
    static int rssi = 0;
    if (esp_wifi_sta_get_rssi(&rssi) == ESP_OK) {
        return rssi;
    } else {
        return 1;
    }
}

// endregion Public functions

static void lock(Wifi* wifi) {
    tt_assert(wifi);
    wifi->mutex.acquire(ms_to_ticks(100));
}

static void unlock(Wifi* wifi) {
    tt_assert(wifi);
    wifi->mutex.release();
}

static void scan_list_alloc(Wifi* wifi) {
    tt_assert(wifi->scan_list == nullptr);
    wifi->scan_list = static_cast<wifi_ap_record_t*>(malloc(sizeof(wifi_ap_record_t) * wifi->scan_list_limit));
    wifi->scan_list_count = 0;
}

static void scan_list_alloc_safely(Wifi* wifi) {
    if (wifi->scan_list == nullptr) {
        scan_list_alloc(wifi);
    }
}

static void scan_list_free(Wifi* wifi) {
    tt_assert(wifi->scan_list != nullptr);
    free(wifi->scan_list);
    wifi->scan_list = nullptr;
    wifi->scan_list_count = 0;
}

static void scan_list_free_safely(Wifi* wifi) {
    if (wifi->scan_list != nullptr) {
        scan_list_free(wifi);
    }
}

static void publish_event_simple(Wifi* wifi, WifiEventType type) {
    WifiEvent turning_on_event = {.type = type};
    tt_pubsub_publish(wifi->pubsub, &turning_on_event);
}

static bool copy_scan_list(Wifi* wifi) {
    if ((wifi->radio_state == WIFI_RADIO_ON || wifi->radio_state == WIFI_RADIO_CONNECTION_ACTIVE) && wifi->scan_active) {
        // Create scan list if it does not exist
        scan_list_alloc_safely(wifi);
        wifi->scan_list_count = 0;
        uint16_t record_count = wifi->scan_list_limit;

        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&record_count, wifi->scan_list));
        uint16_t safe_record_count = TT_MIN(wifi->scan_list_limit, record_count);
        wifi->scan_list_count = safe_record_count;
        TT_LOG_I(TAG, "Scanned %u APs. Showing %u:", record_count, safe_record_count);
        for (uint16_t i = 0; i < safe_record_count; i++) {
            wifi_ap_record_t* record = &wifi->scan_list[i];
            TT_LOG_I(TAG, " - SSID %s (RSSI %d, channel %d)", record->ssid, record->rssi, record->primary);
        }
        return true;
    } else {
        return false;
    }
}

static void auto_connect(Wifi* wifi) {
    for (int i = 0; i < wifi->scan_list_count; ++i) {
        auto ssid = reinterpret_cast<const char*>(wifi->scan_list[i].ssid);
        if (settings::contains(ssid)) {
            static_assert(sizeof(wifi->scan_list[i].ssid) == (TT_WIFI_SSID_LIMIT + 1), "SSID size mismatch");
            settings::WifiApSettings ap_settings;
            if (settings::load(ssid, &ap_settings)) {
                if (ap_settings.auto_connect) {
                    TT_LOG_I(TAG, "Auto-connecting to %s", ap_settings.ssid);
                    connect(&ap_settings, false);
                }
            } else {
                TT_LOG_E(TAG, "Failed to load credentials for ssid %s", ssid);
            }
            break;
        }
    }
}

static void event_handler(TT_UNUSED void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    lock(wifi_singleton);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        TT_LOG_I(TAG, "event_handler: sta start");
        if (wifi_singleton->radio_state == WIFI_RADIO_CONNECTION_PENDING) {
            esp_wifi_connect();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_singleton->radio_state != WIFI_RADIO_OFF_PENDING) {
            wifi_singleton->connection_wait_flags.set(WIFI_FAIL_BIT);
            TT_LOG_I(TAG, "event_handler: disconnected");
            wifi_singleton->radio_state = WIFI_RADIO_ON;
            publish_event_simple(wifi_singleton, WifiEventTypeDisconnected);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(event_data);
        TT_LOG_I(TAG, "event_handler: got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_singleton->connection_wait_flags.set(WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        auto* event = static_cast<wifi_event_sta_scan_done_t*>(event_data);
        TT_LOG_I(TAG, "event_handler: wifi scanning done (scan id %u)", event->scan_id);
        bool copied_list = copy_scan_list(wifi_singleton);

        if (
            wifi_singleton->radio_state != WIFI_RADIO_OFF &&
            wifi_singleton->radio_state != WIFI_RADIO_OFF_PENDING
        ) {
            wifi_singleton->scan_active = false;
            esp_wifi_scan_stop();
        }

        publish_event_simple(wifi_singleton, WifiEventTypeScanFinished);
        TT_LOG_I(TAG, "Finished scan");

        if (copied_list && wifi_singleton->radio_state == WIFI_RADIO_ON) {
            WifiMessage message = {.type = WifiMessageTypeAutoConnect};
            // No need to lock for queue
            wifi_singleton->queue.put(&message, 100 / portTICK_PERIOD_MS);
        }
    }
    unlock(wifi_singleton);
}

static void enable(Wifi* wifi) {
    WifiRadioState state = wifi->radio_state;
    if (
        state == WIFI_RADIO_ON ||
        state == WIFI_RADIO_ON_PENDING ||
        state == WIFI_RADIO_OFF_PENDING
    ) {
        TT_LOG_W(TAG, "Can't enable from current state");
        return;
    }

    TT_LOG_I(TAG, "Enabling");
    wifi->radio_state = WIFI_RADIO_ON_PENDING;
    publish_event_simple(wifi, WifiEventTypeRadioStateOnPending);

    if (wifi->netif != nullptr) {
        esp_netif_destroy(wifi->netif);
    }
    wifi->netif = esp_netif_create_default_wifi_sta();

    // Warning: this is the memory-intensive operation
    // It uses over 117kB of RAM with default settings for S3 on IDF v5.1.2
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t init_result = esp_wifi_init(&config);
    if (init_result != ESP_OK) {
        TT_LOG_E(TAG, "Wifi init failed");
        if (init_result == ESP_ERR_NO_MEM) {
            TT_LOG_E(TAG, "Insufficient memory");
        }
        wifi->radio_state = WIFI_RADIO_OFF;
        publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    // TODO: don't crash on check failure
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        nullptr,
        &wifi->event_handler_any_id
    ));

    // TODO: don't crash on check failure
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        nullptr,
        &wifi->event_handler_got_ip
    ));

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        TT_LOG_E(TAG, "Wifi mode setting failed");
        wifi->radio_state = WIFI_RADIO_OFF;
        esp_wifi_deinit();
        publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    esp_err_t start_result = esp_wifi_start();
    if (start_result != ESP_OK) {
        TT_LOG_E(TAG, "Wifi start failed");
        if (start_result == ESP_ERR_NO_MEM) {
            TT_LOG_E(TAG, "Insufficient memory");
        }
        wifi->radio_state = WIFI_RADIO_OFF;
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_wifi_deinit();
        publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    wifi->radio_state = WIFI_RADIO_ON;
    publish_event_simple(wifi, WifiEventTypeRadioStateOn);
    TT_LOG_I(TAG, "Enabled");
}

static void disable(Wifi* wifi) {
    WifiRadioState state = wifi->radio_state;
    if (
        state == WIFI_RADIO_OFF ||
        state == WIFI_RADIO_OFF_PENDING ||
        state == WIFI_RADIO_ON_PENDING
    ) {
        TT_LOG_W(TAG, "Can't disable from current state");
        return;
    }

    TT_LOG_I(TAG, "Disabling");
    wifi->radio_state = WIFI_RADIO_OFF_PENDING;
    publish_event_simple(wifi, WifiEventTypeRadioStateOffPending);

    // Free up scan list memory
    scan_list_free_safely(wifi_singleton);

    if (esp_wifi_stop() != ESP_OK) {
        TT_LOG_E(TAG, "Failed to stop radio");
        wifi->radio_state = WIFI_RADIO_ON;
        publish_event_simple(wifi, WifiEventTypeRadioStateOn);
        return;
    }

    if (esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to unset mode");
    }

    if (esp_event_handler_instance_unregister(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi->event_handler_any_id
        ) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to unregister id event handler");
    }

    if (esp_event_handler_instance_unregister(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            wifi->event_handler_got_ip
        ) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to unregister ip event handler");
    }

    if (esp_wifi_deinit() != ESP_OK) {
        TT_LOG_E(TAG, "Failed to deinit");
    }

    tt_assert(wifi->netif != nullptr);
    esp_netif_destroy(wifi->netif);
    wifi->netif = nullptr;
    wifi->scan_active = false;
    wifi->radio_state = WIFI_RADIO_OFF;
    publish_event_simple(wifi, WifiEventTypeRadioStateOff);
    TT_LOG_I(TAG, "Disabled");
}

static void scan_internal(Wifi* wifi) {
    WifiRadioState state = wifi->radio_state;
    if (state != WIFI_RADIO_ON && state != WIFI_RADIO_CONNECTION_ACTIVE && state != WIFI_RADIO_CONNECTION_PENDING) {
        TT_LOG_W(TAG, "Scan unavailable: wifi not enabled");
        return;
    }

    if (!wifi->scan_active) {
        if (esp_wifi_scan_start(nullptr, false) == ESP_OK) {
            TT_LOG_I(TAG, "Starting scan");
            wifi->scan_active = true;
            publish_event_simple(wifi, WifiEventTypeScanStarted);
        } else {
            TT_LOG_I(TAG, "Can't start scan");
        }
    } else {
        TT_LOG_W(TAG, "Scan already pending");
    }
}

static void connect_internal(Wifi* wifi) {
    TT_LOG_I(TAG, "Connecting to %s", wifi->connection_target.ssid);

    // Stop radio first, if needed
    WifiRadioState radio_state = wifi->radio_state;
    if (
        radio_state == WIFI_RADIO_ON ||
        radio_state == WIFI_RADIO_CONNECTION_ACTIVE ||
        radio_state == WIFI_RADIO_CONNECTION_PENDING
        ) {
        TT_LOG_I(TAG, "Connecting: Stopping radio first");
        esp_err_t stop_result = esp_wifi_stop();
        wifi->scan_active = false;
        if (stop_result != ESP_OK) {
            TT_LOG_E(TAG, "Connecting: Failed to disconnect (%s)", esp_err_to_name(stop_result));
            return;
        }
    }

    wifi->radio_state = WIFI_RADIO_CONNECTION_PENDING;

    publish_event_simple(wifi, WifiEventTypeConnectionPending);

    wifi_config_t wifi_config = {
        .sta = {
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .ssid = {0},
            .password = {0},
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .bssid_set = false,
            .bssid = { 0 },
            .channel = 0,
            .listen_interval = 0,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold = {
                .rssi = 0,
                .authmode = WIFI_AUTH_WPA2_WPA3_PSK,
            },
            .pmf_cfg = {
                .capable = false,
                .required = false
            },
            .rm_enabled = 0,
            .btm_enabled = 0,
            .mbo_enabled = 0,
            .ft_enabled = 0,
            .owe_enabled = 0,
            .transition_disable = 0,
            .reserved = 0,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .sae_pk_mode = WPA3_SAE_PK_MODE_AUTOMATIC,
            .failure_retry_cnt = 1,
            .he_dcm_set = 0,
            .he_dcm_max_constellation_tx = 0,
            .he_dcm_max_constellation_rx = 0,
            .he_mcs9_enabled = 0,
            .he_su_beamformee_disabled = 0,
            .he_trig_su_bmforming_feedback_disabled = 0,
            .he_trig_mu_bmforming_partial_feedback_disabled = 0,
            .he_trig_cqi_feedback_disabled = 0,
            .he_reserved = 0,
            .sae_h2e_identifier = {0},
        }
    };

    static_assert(sizeof(wifi_config.sta.ssid) == (sizeof(wifi_singleton->connection_target.ssid)-1), "SSID size mismatch");
    memcpy(wifi_config.sta.ssid, wifi_singleton->connection_target.ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, wifi_singleton->connection_target.password, sizeof(wifi_config.sta.password));

    wifi->secure_connection = (wifi_config.sta.password[0] != 0x00);

    esp_err_t set_config_result = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (set_config_result != ESP_OK) {
        wifi->radio_state = WIFI_RADIO_ON;
        TT_LOG_E(TAG, "failed to set wifi config (%s)", esp_err_to_name(set_config_result));
        publish_event_simple(wifi, WifiEventTypeConnectionFailed);
        return;
    }

    esp_err_t wifi_start_result = esp_wifi_start();
    if (wifi_start_result != ESP_OK) {
        wifi->radio_state = WIFI_RADIO_ON;
        TT_LOG_E(TAG, "failed to start wifi to begin connecting (%s)", esp_err_to_name(wifi_start_result));
        publish_event_simple(wifi, WifiEventTypeConnectionFailed);
        return;
    }

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT)
     * or connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
     * The bits are set by wifi_event_handler() */
    uint32_t bits = wifi_singleton->connection_wait_flags.wait(WIFI_FAIL_BIT | WIFI_CONNECTED_BIT);

    if (bits & WIFI_CONNECTED_BIT) {
        wifi->radio_state = WIFI_RADIO_CONNECTION_ACTIVE;
        publish_event_simple(wifi, WifiEventTypeConnectionSuccess);
        TT_LOG_I(TAG, "Connected to %s", wifi->connection_target.ssid);
        if (wifi->connection_target_remember) {
            if (!settings::save(&wifi->connection_target)) {
                TT_LOG_E(TAG, "Failed to store credentials");
            } else {
                TT_LOG_I(TAG, "Stored credentials");
            }
        }
    } else if (bits & WIFI_FAIL_BIT) {
        wifi->radio_state = WIFI_RADIO_ON;
        publish_event_simple(wifi, WifiEventTypeConnectionFailed);
        TT_LOG_I(TAG, "Failed to connect to %s", wifi->connection_target.ssid);
    } else {
        wifi->radio_state = WIFI_RADIO_ON;
        publish_event_simple(wifi, WifiEventTypeConnectionFailed);
        TT_LOG_E(TAG, "UNEXPECTED EVENT");
    }

    wifi_singleton->connection_wait_flags.clear(WIFI_FAIL_BIT | WIFI_CONNECTED_BIT);
}

static void disconnect_internal_but_keep_active(Wifi* wifi) {
    esp_err_t stop_result = esp_wifi_stop();
    if (stop_result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to disconnect (%s)", esp_err_to_name(stop_result));
        return;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = {0},
            .password = {0},
            .threshold = {
                .rssi = 0,
                .authmode = WIFI_AUTH_OPEN,
            },
            .sae_pwe_h2e = WPA3_SAE_PWE_UNSPECIFIED,
            .sae_h2e_identifier = {0},
        },
    };

    esp_err_t set_config_result = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (set_config_result != ESP_OK) {
        // TODO: disable radio, because radio state is in limbo between off and on
        wifi->radio_state = WIFI_RADIO_OFF;
        TT_LOG_E(TAG, "failed to set wifi config (%s)", esp_err_to_name(set_config_result));
        publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    esp_err_t wifi_start_result = esp_wifi_start();
    if (wifi_start_result != ESP_OK) {
        // TODO: disable radio, because radio state is in limbo between off and on
        wifi->radio_state = WIFI_RADIO_OFF;
        TT_LOG_E(TAG, "failed to start wifi to begin connecting (%s)", esp_err_to_name(wifi_start_result));
        publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    wifi->radio_state = WIFI_RADIO_ON;
    publish_event_simple(wifi, WifiEventTypeDisconnected);
    TT_LOG_I(TAG, "Disconnected");
}

// ESP Wi-Fi APIs need to run from the main task, so we can't just spawn a thread
_Noreturn int32_t wifi_main(TT_UNUSED void* parameter) {
    TT_LOG_I(TAG, "Started main loop");
    tt_assert(wifi_singleton != nullptr);
    Wifi* wifi = wifi_singleton;
    MessageQueue& queue = wifi->queue;

    if (TT_WIFI_AUTO_ENABLE) {
        enable(wifi);
        scan_internal(wifi);
    }

    WifiMessage message;
    while (true) {
        if (queue.get(&message, 10000 / portTICK_PERIOD_MS) == TtStatusOk) {
            TT_LOG_I(TAG, "Processing message of type %d", message.type);
            switch (message.type) {
                case WifiMessageTypeRadioOn:
                    lock(wifi);
                    enable(wifi);
                    unlock(wifi);
                    break;
                case WifiMessageTypeRadioOff:
                    lock(wifi);
                    disable(wifi);
                    unlock(wifi);
                    break;
                case WifiMessageTypeScan:
                    lock(wifi);
                    scan_internal(wifi);
                    unlock(wifi);
                    break;
                case WifiMessageTypeConnect:
                    lock(wifi);
                    connect_internal(wifi);
                    unlock(wifi);
                    break;
                case WifiMessageTypeDisconnect:
                    lock(wifi);
                    disconnect_internal_but_keep_active(wifi);
                    unlock(wifi);
                    break;
                case WifiMessageTypeAutoConnect:
                    lock(wifi);
                    auto_connect(wifi_singleton);
                    unlock(wifi);
                    break;
            }
        }

        // Automatic scanning is done so we can automatically connect to access points
        lock(wifi);
        bool should_start_scan = wifi->radio_state == WIFI_RADIO_ON && !wifi->scan_active;
        unlock(wifi);
        if (should_start_scan) {
            scan_internal(wifi);
        }
    }
}

static void service_start(Service& service) {
    tt_assert(wifi_singleton == nullptr);
    wifi_singleton = new Wifi();
    service.setData(wifi_singleton);
}

static void service_stop(Service& service) {
    tt_assert(wifi_singleton != nullptr);

    WifiRadioState state = wifi_singleton->radio_state;
    if (state != WIFI_RADIO_OFF) {
        disable(wifi_singleton);
    }

    delete wifi_singleton;
    wifi_singleton = nullptr;

    // wifi_main() cannot be stopped yet as it runs in the main task.
    // We could theoretically exit it, but then we wouldn't be able to restart the service.
    tt_crash("not fully implemented");
}

extern const Manifest manifest = {
    .id = "Wifi",
    .onStart = &service_start,
    .onStop = &service_stop
};

} // namespace

#endif // ESP_TARGET