#pragma once

#include <string>

namespace tt::service::wifi::settings {

/**
 * This struct is stored as-is into NVS flash.
 *
 * The SSID and secret are increased by 1 byte to facilitate string null termination.
 * This makes it easier to use the char array as a string in various places.
 */
struct WifiApSettings {
    std::string ssid;
    std::string password;
    bool autoConnect;
    int32_t channel;

    WifiApSettings(
        std::string ssid,
        std::string password,
        bool autoConnect = true,
        int32_t channel = 0
    ) : ssid(ssid), password(password), autoConnect(autoConnect), channel(channel) {}

    WifiApSettings() : ssid(""), password(""), autoConnect(true), channel(0) {}
};

/**
 * Check if settings exist for the provided SSID
 * @param[in] ssid the access point to look for
 * @return true if the settings exist
 */
bool contains(const std::string& ssid);

/**
 * Load the settings for the provided SSID
 * @param[in] ssid the access point to look for
 * @param[out] settings the output settings
 * @return true if the settings were loaded successfully
 */
bool load(const std::string& ssid, WifiApSettings& settings);

/**
 * Save settings
 * @param settings the settings to save
 * @return true when the settings were saved successfully
 */
bool save(const WifiApSettings& settings);

/**
 * Remove settings that were saved previously.
 * @param ssid the name of the SSID for the settings to remove
 * @return true when settings were found and removed
 */
bool remove(const std::string& ssid);

} // namespace
