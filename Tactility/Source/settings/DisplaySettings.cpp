#include <Tactility/settings/DisplaySettings.h>

#include <Tactility/file/PropertiesFile.h>
#include <Tactility/hal/Device.h>
#include <Tactility/hal/display/DisplayDevice.h>

#include <map>
#include <string>
#include <utility>

namespace tt::settings::display {

constexpr auto* TAG = "DisplaySettings";
constexpr auto* SETTINGS_FILE = "/data/settings/display.properties";
constexpr auto* SETTINGS_KEY_ORIENTATION = "orientation";
constexpr auto* SETTINGS_KEY_GAMMA_CURVE = "gammaCurve";
constexpr auto* SETTINGS_KEY_BACKLIGHT_DUTY = "backlightDuty";

static Orientation getDefaultOrientation() {
    auto* display = lv_display_get_default();
    if (display == nullptr) {
        return Orientation::Landscape;
    }

    if (lv_display_get_physical_horizontal_resolution(display) > lv_display_get_physical_vertical_resolution(display)) {
        return Orientation::Landscape;
    } else {
        return Orientation::Portrait;
    }
}

static std::string toString(Orientation orientation) {
    switch (orientation) {
        using enum Orientation;
        case Portrait:
            return "Portrait";
        case Landscape:
            return "Landscape";
        case PortraitFlipped:
            return "PortraitFlipped";
        case LandscapeFlipped:
            return "LandscapeFlipped";
        default:
            std::unreachable();
    }
}

static bool fromString(const std::string& str, Orientation& orientation) {
    if (str == "Portrait") {
        orientation = Orientation::Portrait;
        return true;
    } else if (str == "Landscape") {
        orientation = Orientation::Landscape;
        return true;
    } else if (str == "PortraitFlipped") {
        orientation = Orientation::PortraitFlipped;
        return true;
    } else if (str == "LandscapeFlipped") {
        orientation = Orientation::LandscapeFlipped;
        return true;
    } else {
        return false;
    }
}

bool load(DisplaySettings& settings) {
    std::map<std::string, std::string> map;
    if (!file::loadPropertiesFile(SETTINGS_FILE, map)) {
        return false;
    }

    auto orientation_entry = map.find(SETTINGS_KEY_ORIENTATION);
    Orientation orientation;
    if (orientation_entry == map.end() || !fromString(orientation_entry->second, orientation)) {
        orientation = getDefaultOrientation();
    }

    auto gamma_entry = map.find(SETTINGS_KEY_GAMMA_CURVE);
    int gamma_curve = 0;
    if (gamma_entry != map.end()) {
        gamma_curve = atoi(gamma_entry->second.c_str());
    }

    auto backlight_duty_entry = map.find(SETTINGS_KEY_BACKLIGHT_DUTY);
    int backlight_duty = 200; // default
    if (backlight_duty_entry != map.end()) {
        backlight_duty = atoi(backlight_duty_entry->second.c_str());
        if (backlight_duty_entry->second != "0" && backlight_duty == 0) {
            backlight_duty = 200;
        }
    }

    settings.orientation = orientation;
    settings.gammaCurve = gamma_curve;
    settings.backlightDuty = backlight_duty;

    return true;
}

DisplaySettings getDefault() {
    return DisplaySettings {
        .orientation = getDefaultOrientation(),
        .gammaCurve = 1,
        .backlightDuty = 200
    };
}

DisplaySettings loadOrGetDefault() {
    DisplaySettings settings;
    if (!load(settings)) {
        settings = getDefault();
    }
    return settings;
}

bool save(const DisplaySettings& settings) {
    std::map<std::string, std::string> map;
    map[SETTINGS_KEY_BACKLIGHT_DUTY] = std::to_string(settings.backlightDuty);
    map[SETTINGS_KEY_GAMMA_CURVE] = std::to_string(settings.gammaCurve);
    map[SETTINGS_KEY_ORIENTATION] = toString(settings.orientation);
    return file::savePropertiesFile(SETTINGS_FILE, map);
}

lv_display_rotation_t toLvglDisplayRotation(Orientation orientation) {
    auto* lvgl_display = lv_display_get_default();
    auto rotation = lv_display_get_rotation(lvgl_display);
    bool is_originally_landscape;
    // The lvgl resolution code compensates for rotation. We have to revert the compensation to get the real display resolution
    // TODO: Use info from display driver
    if (rotation == LV_DISPLAY_ROTATION_0 || rotation == LV_DISPLAY_ROTATION_180) {
        is_originally_landscape = lv_display_get_physical_horizontal_resolution(lvgl_display) > lv_display_get_physical_vertical_resolution(lvgl_display);
    } else {
        is_originally_landscape = lv_display_get_physical_horizontal_resolution(lvgl_display) < lv_display_get_physical_vertical_resolution(lvgl_display);
    }
    if (is_originally_landscape) {
        // Landscape display
        switch (orientation) {
            case Orientation::Landscape:
                return LV_DISPLAY_ROTATION_0;
            case Orientation::Portrait:
                return LV_DISPLAY_ROTATION_90;
            case Orientation::LandscapeFlipped:
                return LV_DISPLAY_ROTATION_180;
            case Orientation::PortraitFlipped:
                return LV_DISPLAY_ROTATION_270;
            default:
                return LV_DISPLAY_ROTATION_0;
        }
    } else {
        // Portrait display
        switch (orientation) {
            case Orientation::Landscape:
                return LV_DISPLAY_ROTATION_90;
            case Orientation::Portrait:
                return LV_DISPLAY_ROTATION_0;
            case Orientation::LandscapeFlipped:
                return LV_DISPLAY_ROTATION_270;
            case Orientation::PortraitFlipped:
                return LV_DISPLAY_ROTATION_180;
            default:
                return LV_DISPLAY_ROTATION_0;
        }
    }
}

} // namespace
