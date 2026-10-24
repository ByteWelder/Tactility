#include <Tactility/app/AppIcon.h>

#include <lvgl/icons/shared.h>

#include <cstring>

namespace tt::app {

namespace {

struct IconEntry {
    const char* id;
    const char* icon;
};

// The apps of the launcher's apps mode, its settings mode, and the apps the system itself shows
// (the launcher in the recents switcher)
constexpr IconEntry ICONS[] = {
    {"tactility.apphub",            LVGL_ICON_SHARED_DOWNLOAD},
    {"tactility.camera",            LVGL_ICON_SHARED_CAMERA},
    {"tactility.chat",              LVGL_ICON_SHARED_FORUM},
    {"tactility.files",             LVGL_ICON_SHARED_FOLDER},
    {"tactility.i2cscanner",        LVGL_ICON_SHARED_CABLE},
    {"tactility.launcher",          LVGL_ICON_SHARED_APPS},
    {"tactility.meshchannels",      LVGL_ICON_SHARED_FORUM},
    {"tactility.meshcontacts",      LVGL_ICON_SHARED_FORUM},
    {"tactility.notes",             LVGL_ICON_SHARED_EDIT_NOTE},
    {"tactility.systeminfo",        LVGL_ICON_SHARED_DEVICES},
    {"tactility.terminal",          LVGL_ICON_SHARED_TERMINAL},

    {"tactility.appearance",        LVGL_ICON_SHARED_PALETTE},
    {"tactility.audiosettings",     LVGL_ICON_SHARED_MUSIC_NOTE},
    {"tactility.btmanage",          LVGL_ICON_SHARED_BLUETOOTH},
    {"tactility.development",       LVGL_ICON_SHARED_LOGO_DEV},
    {"tactility.display",           LVGL_ICON_SHARED_DISPLAY_SETTINGS},
    {"tactility.grovesettings",     LVGL_ICON_SHARED_CABLE},
    {"tactility.keyboardsettings",  LVGL_ICON_SHARED_KEYBOARD_ALT},
    {"tactility.ledstripsettings",  LVGL_ICON_SHARED_LIGHTSTRIP},
    {"tactility.locationsettings",  LVGL_ICON_SHARED_LOCATION_ON},
    {"tactility.meshsettings",      LVGL_ICON_SHARED_HUB},
    {"tactility.power",             LVGL_ICON_SHARED_POWER_SETTINGS_NEW},
    {"tactility.timedatesettings",  LVGL_ICON_SHARED_CALENDAR_MONTH},
    {"tactility.touchcalibration",  LVGL_ICON_SHARED_CIRCLE},
    {"tactility.trackballsettings", LVGL_ICON_SHARED_DEVICES},
    {"tactility.usbsettings",       LVGL_ICON_SHARED_USB},
    {"tactility.wifimanage",        LVGL_ICON_SHARED_WIFI},
};

}

const char* getAppIcon(const ::AppManifest& manifest) {
    for (const auto& entry : ICONS) {
        if (!strcmp(manifest.id, entry.id)) return entry.icon;
    }
    return LVGL_ICON_SHARED_DEPLOYED_CODE;
}

}
