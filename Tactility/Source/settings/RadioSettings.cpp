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

RadioRegion findRegionForTimeZone(const std::string& timeZoneName) {
    struct ZoneRegion {
        const char* zone;
        RadioRegion region;
    };
    // Exact names before the area prefixes below, e.g. the Russian zones in Europe and Asia
    static constexpr ZoneRegion ZONES[] = {
        { "Europe/Moscow", RADIO_REGION_RU_868 },
        { "Europe/Kaliningrad", RADIO_REGION_RU_868 },
        { "Europe/Samara", RADIO_REGION_RU_868 },
        { "Europe/Volgograd", RADIO_REGION_RU_868 },
        { "Europe/Kirov", RADIO_REGION_RU_868 },
        { "Europe/Astrakhan", RADIO_REGION_RU_868 },
        { "Europe/Saratov", RADIO_REGION_RU_868 },
        { "Europe/Ulyanovsk", RADIO_REGION_RU_868 },
        { "Asia/Yekaterinburg", RADIO_REGION_RU_868 },
        { "Asia/Omsk", RADIO_REGION_RU_868 },
        { "Asia/Novosibirsk", RADIO_REGION_RU_868 },
        { "Asia/Barnaul", RADIO_REGION_RU_868 },
        { "Asia/Tomsk", RADIO_REGION_RU_868 },
        { "Asia/Novokuznetsk", RADIO_REGION_RU_868 },
        { "Asia/Krasnoyarsk", RADIO_REGION_RU_868 },
        { "Asia/Irkutsk", RADIO_REGION_RU_868 },
        { "Asia/Chita", RADIO_REGION_RU_868 },
        { "Asia/Yakutsk", RADIO_REGION_RU_868 },
        { "Asia/Khandyga", RADIO_REGION_RU_868 },
        { "Asia/Vladivostok", RADIO_REGION_RU_868 },
        { "Asia/Ust-Nera", RADIO_REGION_RU_868 },
        { "Asia/Magadan", RADIO_REGION_RU_868 },
        { "Asia/Sakhalin", RADIO_REGION_RU_868 },
        { "Asia/Srednekolymsk", RADIO_REGION_RU_868 },
        { "Asia/Kamchatka", RADIO_REGION_RU_868 },
        { "Asia/Anadyr", RADIO_REGION_RU_868 },
        { "Asia/Shanghai", RADIO_REGION_CN_470 },
        { "Asia/Urumqi", RADIO_REGION_CN_470 },
        { "Asia/Kolkata", RADIO_REGION_IN_865 },
        { "Asia/Calcutta", RADIO_REGION_IN_865 },
        { "Asia/Tokyo", RADIO_REGION_JP_920 },
        { "Pacific/Auckland", RADIO_REGION_NZ_865 },
        { "Pacific/Chatham", RADIO_REGION_NZ_865 },
    };
    static constexpr ZoneRegion AREAS[] = {
        { "Europe/", RADIO_REGION_EU_868 },
        { "America/", RADIO_REGION_US_915 },
        { "Australia/", RADIO_REGION_AU_915 },
    };
    for (const auto& zone : ZONES) {
        if (timeZoneName == zone.zone) {
            return zone.region;
        }
    }
    for (const auto& area : AREAS) {
        if (timeZoneName.starts_with(area.zone)) {
            return area.region;
        }
    }
    return RADIO_REGION_UNSET;
}

}
