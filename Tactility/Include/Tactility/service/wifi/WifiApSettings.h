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

bool contains(const std::string& ssid);

bool load(const std::string& ssid, WifiApSettings& settings);

bool save(const WifiApSettings& settings);

bool remove(const std::string& ssid);

} // namespace
