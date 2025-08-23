#pragma once

namespace tt::service::wifi {

/**
 * Called during boot, this function loads WiFi settings from SD card (when available).
 */
void bootSplashInit();

}