#include "network/NtpPrivate.h"
#include "kernel/SystemEvents.h"
#include "TactilityCore.h"

#ifdef ESP_PLATFORM
#include <esp_netif_sntp.h>
#include <esp_sntp.h>

#endif

#define TAG "ntp"

namespace tt::network::ntp {

#ifdef ESP_PLATFORM

static void onTimeSynced(struct timeval* tv) {
    TT_LOG_I(TAG, "Time synced (%llu)", tv->tv_sec);
    kernel::systemEventPublish(kernel::SystemEvent::NtpSynced);
}

void init() {
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.sync_cb = onTimeSynced;
    esp_netif_sntp_init(&config);
}

#else

void init() {
}

#endif

}
