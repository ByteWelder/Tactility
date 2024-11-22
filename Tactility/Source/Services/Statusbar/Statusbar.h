#pragma once

namespace tt::service::statusbar {

/**
 * Return the relevant icon asset from assets.h for the given inputs
 * @param rssi the rssi value
 * @param secured whether the access point is a secured one (as in: not an open one)
 * @return
 */
const char* get_status_icon_for_rssi(int rssi, bool secured);

} // namespace
