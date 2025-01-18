#include "TactilityCore.h"
#include "UnPhoneFeatures.h"
#include <esp_sleep.h>

#define TAG "unphone"

UnPhoneFeatures unPhoneFeatures;

static std::unique_ptr<tt::Thread> powerThread;

static void updatePowerSwitch() {
    static bool last_on_state = true;

    if (!unPhoneFeatures.isPowerSwitchOn()) {
        if (last_on_state) {
            TT_LOG_W(TAG, "Power off");
        }

        unPhoneFeatures.turnPeripheralsOff();

        if (!unPhoneFeatures.isUsbPowerConnected()) { // and usb unplugged we go into shipping mode
            if (last_on_state) {
                TT_LOG_W(TAG, "Shipping mode until USB connects");
                unPhoneFeatures.setShipping(true); // tell BM to stop supplying power until USB connects
            }
        } else { // power switch off and usb plugged in we sleep
            unPhoneFeatures.wakeOnPowerSwitch();
            esp_sleep_enable_timer_wakeup(60000000); // ea min: USB? else->shipping
            esp_deep_sleep_start(); // deep sleep, wait for wakeup on GPIO
        }

        last_on_state = false;
    } else {
        if (!last_on_state) {
            TT_LOG_W(TAG, "Power on");
            unPhoneFeatures.setShipping(false);
        }
        last_on_state = true;
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
    if (!unPhoneFeatures.init()) {
        TT_LOG_E(TAG, "UnPhoneFeatures init failed");
        return false;
    }

    unPhoneFeatures.printInfo();

    updatePowerSwitch();
    startPowerSwitchThread();

    unPhoneFeatures.setBacklightPower(false);

    // Init touch screen GPIOs (used for vibe motor)
    unPhoneFeatures.setVibePower(false);

    unPhoneFeatures.setIrPower(false);

    // This will be default LOW implicitly, but this makes it explicit
    unPhoneFeatures.setExpanderPower(false);

    for (int i = 0; i < 3; i++) { // buzz a bit
        unPhoneFeatures.setVibePower(true);
        tt::kernel::delayMillis(150);
        unPhoneFeatures.setVibePower(false);
        tt::kernel::delayMillis(150);
    }

    unPhoneFeatures.setBacklightPower(true);

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
