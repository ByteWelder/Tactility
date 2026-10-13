#include <Tactility/settings/RadioSettings.h>

#include <tactility/device.h>
#include <tactility/radio_settings.h>

namespace tt::settings::radio {

RadioRegion loadRegion() {
    return radio_settings_load_region();
}

bool saveRegion(RadioRegion region) {
    return radio_settings_save_region(region) == ERROR_NONE;
}

void applyRegion() {
    radio_settings_apply_region();
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
