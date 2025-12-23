#include <Tactility/service/ServiceRegistration.h>
#include <Tactility/service/ServiceManifest.h>
#include <Tactility/service/ServiceContext.h>
#include <Tactility/settings/KeyboardSettings.h>
#include <Tactility/TactilityCore.h>
#include <Tactility/Log.h>

// Forward declare driver functions
namespace driver::keyboardbacklight {
    bool setBrightness(uint8_t brightness);
}

namespace driver::trackball {
    void setEnabled(bool enabled);
}

namespace tt::service::keyboardinit {

constexpr auto* TAG = "KeyboardInit";

class KeyboardInitService final : public Service {
public:
    bool onStart(TT_UNUSED ServiceContext& service) override {
        auto settings = settings::keyboard::loadOrGetDefault();
        
        // Apply keyboard backlight setting
        bool result = driver::keyboardbacklight::setBrightness(settings.backlightEnabled ? settings.backlightBrightness : 0);
        if (!result) {
            TT_LOG_W(TAG, "Failed to set keyboard backlight brightness");
        }
        
        // Apply trackball enabled setting
        driver::trackball::setEnabled(settings.trackballEnabled);
        
        return true;
    }

    void onStop(TT_UNUSED ServiceContext& service) override {
        // Nothing to clean up
    }
};

extern const ServiceManifest manifest = {
    .id = "KeyboardInit",
    .createService = create<KeyboardInitService>
};

}
