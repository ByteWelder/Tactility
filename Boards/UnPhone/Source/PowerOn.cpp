#include "Preferences.h"
#include "TactilityCore.h"
#include "UnPhoneFeatures.h"
#include <esp_sleep.h>

#define TAG "unphone"

extern UnPhoneFeatures unPhoneFeatures;

static std::unique_ptr<tt::Thread> powerThread;

static const char* bootCountKey = "boot_count";
static const char* powerOffCountKey = "power_off_count";
static const char* powerSleepKey = "power_sleep_key";

class DeviceStats {

private:

    tt::Preferences preferences = tt::Preferences("unphone");

    int32_t getValue(const char* key) {
        int32_t value = 0;
        preferences.optInt32(key, value);
        return value;
    }

    void setValue(const char* key, int32_t value) {
        preferences.putInt32(key, value);
    }

    void increaseValue(const char* key) {
        int32_t new_value = getValue(key) + 1;
        setValue(key, new_value);
    }

public:

    void notifyBootStart() {
        increaseValue(bootCountKey);
    }

    void notifyPowerOff() {
        increaseValue(powerOffCountKey);
    }

    void notifyPowerSleep() {
        increaseValue(powerSleepKey);
    }

    void printInfo() {
        TT_LOG_I("TAG", "Device stats:");
        TT_LOG_I("TAG", "  boot: %ld", getValue(bootCountKey));
        TT_LOG_I("TAG", "  power off: %ld", getValue(powerOffCountKey));
        TT_LOG_I("TAG", "  power sleep: %ld", getValue(powerSleepKey));
    }
};

DeviceStats bootStats;

enum class PowerState {
    Initial,
    On,
    Off
};

#define DEBUG_POWER_STATES false

#if DEBUG_POWER_STATES
/** Helper method to use the buzzer to signal the different power stages */
static void powerInfoBuzz(uint8_t count) {
    if (DEBUG_POWER_STATES) {
        uint8_t index = 0;
        while (index < count) {
            unPhoneFeatures.setVibePower(true);
            tt::kernel::delayMillis(50);
            unPhoneFeatures.setVibePower(false);

            index++;

            if (index < count) {
                tt::kernel::delayMillis(100);
            }
        }
    }
}
#endif

static void updatePowerSwitch() {
    static PowerState last_state = PowerState::Initial;

    if (!unPhoneFeatures.isPowerSwitchOn()) {
        if (last_state != PowerState::Off) {
            last_state = PowerState::Off;
            TT_LOG_W(TAG, "Power off");
        }

        if (!unPhoneFeatures.isUsbPowerConnected()) { // and usb unplugged we go into shipping mode
            TT_LOG_W(TAG, "Shipping mode until USB connects");

#if DEBUG_POWER_STATES
            unPhoneFeatures.setExpanderPower(true);
            powerInfoBuzz(3);
            unPhoneFeatures.setExpanderPower(false);
#endif

            unPhoneFeatures.turnPeripheralsOff();

            bootStats.notifyPowerOff();

            unPhoneFeatures.setShipping(true); // tell BM to stop supplying power until USB connects
        } else { // When power switch is off, but USB is plugged in, we wait (deep sleep) until USB is unplugged.
            TT_LOG_W(TAG, "Waiting for USB disconnect to power off");

#if DEBUG_POWER_STATES
            powerInfoBuzz(2);
#endif

            unPhoneFeatures.turnPeripheralsOff();

            bootStats.notifyPowerSleep();

            // Deep sleep for 1 minute, then awaken to check power state again
            // GPIO trigger from power switch also awakens the device
            unPhoneFeatures.wakeOnPowerSwitch();
            esp_sleep_enable_timer_wakeup(60000000);
            esp_deep_sleep_start();
        }
    } else {
        if (last_state != PowerState::On) {
            last_state = PowerState::On;
            TT_LOG_W(TAG, "Power on");

#if DEBUG_POWER_STATES
            powerInfoBuzz(1);
#endif
        }
    }
}

static int32_t powerSwitchMain(void*) { // check power switch every 10th of sec
    while (true) {
        updatePowerSwitch();
        tt::kernel::delayMillis(200);
    }
}

static void startPowerSwitchThread() {
    powerThread = std::make_unique<tt::Thread>(
        "unphone_power_switch",
        4096,
        powerSwitchMain,
        nullptr
    );
    powerThread->start();
}

static bool unPhonePowerOn() {
    // Print early, in case of early crash (info will be from previous boot)
    bootStats.printInfo();

    bootStats.notifyBootStart();

    if (!unPhoneFeatures.init()) {
        TT_LOG_E(TAG, "UnPhoneFeatures init failed");
        return false;
    }

    unPhoneFeatures.printInfo();

    unPhoneFeatures.setBacklightPower(false);
    unPhoneFeatures.setVibePower(false);
    unPhoneFeatures.setIrPower(false);
    unPhoneFeatures.setExpanderPower(false);

    // Turn off the device if power switch is on off state,
    // instead of waiting for the Thread to start and continue booting
    updatePowerSwitch();
    startPowerSwitchThread();

    return true;
}

bool unPhoneInitPower() {
    ESP_LOGI(TAG, LOG_MESSAGE_POWER_ON_START);

    if (!unPhonePowerOn()) {
        TT_LOG_E(TAG, LOG_MESSAGE_POWER_ON_FAILED);
        return false;
    }

    return true;
}
