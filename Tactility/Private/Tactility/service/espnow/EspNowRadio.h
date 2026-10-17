#pragma once

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#if (defined(CONFIG_SOC_WIFI_SUPPORTED) || defined(CONFIG_ESP_HOSTED_ENABLED))

namespace tt::service::espnow {

/**
 * Adds a radio device (RADIO_TYPE) that sends and receives broadcast frames with ESP-NOW, so that the mesh service
 * can use it like a LoRa radio. It uses the ESP-NOW service, so it must be registered after that service.
 */
void registerRadio();

}

#endif
