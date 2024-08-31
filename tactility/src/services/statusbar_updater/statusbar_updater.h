#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return the relevant icon asset from assets.h for the given inputs
 * @param rssi the rssi value
 * @param secured whether the access point is a secured one (as in: not an open one)
 * @return
 */
const char* wifi_get_status_icon_for_rssi(int rssi, bool secured);

#ifdef __cplusplus
}
#endif