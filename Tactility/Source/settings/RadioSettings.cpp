#include <Tactility/DeprecatedPaths.h>
#include <Tactility/file/File.h>
#include <Tactility/file/PropertiesFile.h>
#include <Tactility/settings/RadioSettings.h>

#include <tactility/device.h>
#include <tactility/log.h>

#include <cstring>
#include <format>
#include <map>
#include <string>

namespace tt::settings::radio {

constexpr auto* TAG = "RadioSettings";
constexpr auto* FILE_PATH_FORMAT = "{}/settings/radio.properties";
constexpr auto* KEY_REGION = "region";

static std::string getFilePath() {
    return std::format(FILE_PATH_FORMAT, getDataPath());
}

RadioRegion loadRegion() {
    const auto path = getFilePath();
    std::map<std::string, std::string> map;
    if (!file::isFile(path) || !file::loadPropertiesFile(path, map)) {
        return RADIO_REGION_UNSET;
    }
    const auto entry = map.find(KEY_REGION);
    if (entry == map.end()) {
        return RADIO_REGION_UNSET;
    }
    for (int region = RADIO_REGION_UNSET; region < RADIO_REGION_COUNT; region++) {
        if (entry->second == radio_region_to_string(static_cast<RadioRegion>(region))) {
            return static_cast<RadioRegion>(region);
        }
    }
    LOG_W(TAG, "Unknown region \"%s\"", entry->second.c_str());
    return RADIO_REGION_UNSET;
}

static void applyToDevices(RadioRegion region) {
    device_for_each_of_type(&RADIO_TYPE, &region, [](Device* device, void* context) {
        radio_set_region(device, *static_cast<RadioRegion*>(context));
        return true;
    });
}

bool saveRegion(RadioRegion region) {
    const auto path = getFilePath();
    std::map<std::string, std::string> map;
    map[KEY_REGION] = radio_region_to_string(region);
    if (!file::findOrCreateParentDirectory(path, 0755) || !file::savePropertiesFile(path, map)) {
        LOG_E(TAG, "Failed to save %s", path.c_str());
        return false;
    }
    applyToDevices(region);
    return true;
}

void applyRegion() {
    const auto region = loadRegion();
    if (region != RADIO_REGION_UNSET) {
        LOG_I(TAG, "Region %s", radio_region_to_string(region));
        applyToDevices(region);
    }
}

const char* getRegionLabel(RadioRegion region) {
    switch (region) {
        case RADIO_REGION_EU_433:
            return "Europe 433 MHz";
        case RADIO_REGION_EU_868:
            return "Europe 868 MHz";
        case RADIO_REGION_US_915:
            return "United States 915 MHz";
        case RADIO_REGION_AU_915:
            return "Australia 915 MHz";
        case RADIO_REGION_NZ_865:
            return "New Zealand 865 MHz";
        case RADIO_REGION_IN_865:
            return "India 865 MHz";
        case RADIO_REGION_RU_868:
            return "Russia 868 MHz";
        case RADIO_REGION_CN_470:
            return "China 470 MHz";
        case RADIO_REGION_JP_920:
            return "Japan 920 MHz";
        default:
            return "Not set";
    }
}

bool hasRadio() {
    return device_exists_of_type(&RADIO_TYPE);
}

}
