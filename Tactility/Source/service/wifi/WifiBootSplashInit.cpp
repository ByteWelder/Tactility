#include "Tactility/service/wifi/WifiBootSplashInit.h"
#include "Tactility/file/PropertiesFile.h"

#include <Tactility/file/File.h>
#include <Tactility/Log.h>
#include <Tactility/service/wifi/WifiApSettings.h>

#include <dirent.h>
#include <format>
#include <map>
#include <string>
#include <vector>
#include <Tactility/Tactility.h>
#include <Tactility/hal/sdcard/SdCardDevice.h>

namespace tt::service::wifi {

constexpr auto* TAG = "WifiBootSplashInit";

constexpr auto* AP_PROPERTIES_KEY_SSID = "ssid";
constexpr auto* AP_PROPERTIES_KEY_PASSWORD = "password";
constexpr auto* AP_PROPERTIES_KEY_AUTO_CONNECT = "autoConnect";
constexpr auto* AP_PROPERTIES_KEY_CHANNEL = "channel";
constexpr auto* AP_PROPERTIES_KEY_AUTO_REMOVE = "autoRemovePropertiesFile";

struct ApProperties {
    std::string ssid;
    std::string password;
    bool autoConnect;
    int32_t channel;
    bool autoRemovePropertiesFile;
};

static void importWifiAp(const std::string& filePath) {
    std::map<std::string, std::string> map;
    if (!file::loadPropertiesFile(filePath, map)) {
        TT_LOG_E(TAG, "Failed to load AP properties at %s", filePath.c_str());
        return;
    }

    const auto ssid_iterator = map.find(AP_PROPERTIES_KEY_SSID);
    if (ssid_iterator == map.end()) {
        TT_LOG_E(TAG, "%s is missing ssid", filePath.c_str());
        return;
    }
    const auto ssid = ssid_iterator->second;

    if (!settings::contains(ssid)) {

        const auto password_iterator = map.find(AP_PROPERTIES_KEY_PASSWORD);
        const auto password = password_iterator == map.end() ? "" : password_iterator->second;

        const auto auto_connect_iterator = map.find(AP_PROPERTIES_KEY_AUTO_CONNECT);
        const auto auto_connect = auto_connect_iterator == map.end() ? true : (auto_connect_iterator->second == "true");

        const auto channel_iterator = map.find(AP_PROPERTIES_KEY_CHANNEL);
        const auto channel = channel_iterator == map.end() ? 0 : std::stoi(channel_iterator->second);

        settings::WifiApSettings settings(
            ssid,
            password,
            auto_connect,
            channel
        );

        if (!settings::save(settings)) {
            TT_LOG_E(TAG, "Failed to save settings for %s", ssid.c_str());
        } else {
            TT_LOG_I(TAG, "Imported %s from %s", ssid.c_str(), filePath.c_str());
        }
    }

    const auto auto_remove_iterator = map.find(AP_PROPERTIES_KEY_AUTO_REMOVE);
    if (auto_remove_iterator != map.end() && auto_remove_iterator->second == "true") {
        if (!remove(filePath.c_str())) {
            TT_LOG_E(TAG, "Failed to auto-remove %s", filePath.c_str());
        } else {
            TT_LOG_I(TAG, "Auto-removed %s", filePath.c_str());
        }
    }
}

static void importWifiApSettings(std::shared_ptr<hal::sdcard::SdCardDevice> sdcard) {
    auto path = file::getChildPath(sdcard->getMountPath(), "settings");

    std::vector<dirent> dirent_list;
    if (file::scandir(path, dirent_list, [](const dirent* entry) {
        switch (entry->d_type) {
            case file::TT_DT_DIR:
            case file::TT_DT_CHR:
            case file::TT_DT_LNK:
                return -1;
            case file::TT_DT_REG:
            default: {
                std::string name = entry->d_name;
                if (name.ends_with(".ap.properties")) {
                    return 0;
                } else {
                    return -1;
                }
            }
        }
    }, nullptr) == 0) {
        return;
    }

    if (dirent_list.empty()) {
        TT_LOG_W(TAG, "No AP files found at %s", sdcard->getMountPath().c_str());
        return;
    }

    for (auto& dirent : dirent_list) {
        std::string absolute_path = std::format("{}/{}", path, dirent.d_name);
        importWifiAp(absolute_path);
    }
}

void bootSplashInit() {
    getMainDispatcher().dispatch([] {
        const auto sdcards = hal::findDevices<hal::sdcard::SdCardDevice>(hal::Device::Type::SdCard);
        for (auto& sdcard : sdcards) {
            if (sdcard->isMounted()) {
                importWifiApSettings(sdcard);
            } else {
                TT_LOG_W(TAG, "Skipping unmounted SD card %s", sdcard->getMountPath().c_str());
            }
        }
    });
}

}
